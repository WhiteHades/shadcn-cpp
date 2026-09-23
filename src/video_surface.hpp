// SPDX-License-Identifier: MIT
#pragma once

#include <QRhiWidget>
#include <QFile>
#include <QVideoSink>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qvideoframetexturepool_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtGui/rhi/qrhi.h>

#include <algorithm>
#include <array>
#include <memory>

namespace shadcn::detail {

class VideoSurface final : public QRhiWidget {
public:
    explicit VideoSurface(QWidget* parent = nullptr) : QRhiWidget(parent) {
        connect(&sink_, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
            const auto oldFrame = pool_->currentFrame();
            pool_->setCurrentFrame(frame);
            if (!frame.isValid() || !oldFrame.isValid() || oldFrame.surfaceFormat() != frame.surfaceFormat()) {
                renderFailed_ = false;
                pipeline_.reset();
                bindings_.reset();
                vertexShaderName_.clear();
                fragmentShaderName_.clear();
            }
            update();
        });
        connect(this, &QRhiWidget::frameSubmitted, this, [this] {
            pool_->onFrameEndInvoked();
        });
    }

    ~VideoSurface() override { releaseResources(); }

    [[nodiscard]] QVideoSink* videoSink() noexcept { return &sink_; }

    [[nodiscard]] Qt::AspectRatioMode aspectRatioMode() const noexcept { return aspectRatioMode_; }

    void setAspectRatioMode(Qt::AspectRatioMode mode) {
        if (aspectRatioMode_ == mode)
            return;
        aspectRatioMode_ = mode;
        update();
    }

protected:
    void releaseResources() override {
        sink_.setRhi(nullptr);
        pipeline_.reset();
        bindings_.reset();
        sampler_.reset();
        vertices_.reset();
        uniforms_.reset();
        vertexShaderName_.clear();
        fragmentShaderName_.clear();
        pipelineSampleCount_ = 0;
        renderFailed_ = false;

        const auto frame = pool_->currentFrame();
        pool_ = std::make_unique<QVideoFrameTexturePool>();
        pool_->setCurrentFrame(frame);
        context_ = nullptr;
    }

    void initialize(QRhiCommandBuffer*) override {
        if (context_ != rhi())
            releaseResources();
        context_ = rhi();
        if (!context_)
            return;
        sink_.setRhi(context_);

        if (vertices_)
            return;

        vertices_.reset(context_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer,
                                            static_cast<quint32>(sizeof(float) * 16)));
        uniforms_.reset(context_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                            static_cast<quint32>(sizeof(QVideoTextureHelper::UniformData))));
        sampler_.reset(context_->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                            QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        if (!vertices_ || !uniforms_ || !sampler_ || !vertices_->create() || !uniforms_->create() || !sampler_->create()) {
            vertices_.reset();
            uniforms_.reset();
            sampler_.reset();
            reportFailure("failed to create video rendering resources");
            return;
        }
    }

    void render(QRhiCommandBuffer* commandBuffer) override {
        if (!context_ || !renderTarget() || !commandBuffer)
            return;

        auto* updates = context_->nextResourceUpdateBatch();
        bool draw = false;
        const auto& frame = pool_->currentFrame();
        const auto format = frame.surfaceFormat();
        const auto* description = frame.isValid() ? QVideoTextureHelper::textureDescription(format.pixelFormat()) : nullptr;
        QVideoFrameTextures* textures = nullptr;

        if (!renderFailed_ && vertices_ && uniforms_ && sampler_ && frame.isValid() && format.isValid() && !format.frameSize().isEmpty() && description && description->nplanes > 0) {
            textures = pool_->updateTextures(*context_, *updates);
            if (!textures) {
                reportFailure("failed to create video textures");
            } else {
                QList<QRhiShaderResourceBinding> resources;
                resources.append(QRhiShaderResourceBinding::uniformBuffer(
                        0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                        uniforms_.get()));
                bool validTextures = true;
                for (int plane = 0; plane < description->nplanes; ++plane) {
                    auto* texture = textures->texture(static_cast<uint>(plane));
                    if (!texture) {
                        validTextures = false;
                        break;
                    }
                    resources.append(QRhiShaderResourceBinding::sampledTexture(
                            plane + 1, QRhiShaderResourceBinding::FragmentStage, texture, sampler_.get()));
                }
                if (!validTextures) {
                    reportFailure("video texture plane is missing");
                } else {
                    const auto vertexShader = QVideoTextureHelper::vertexShaderFileName(format);
                    const auto fragmentShader = QVideoTextureHelper::fragmentShaderFileName(format, context_);
                    const int sampleCount = renderTarget()->sampleCount();
                    const bool rebuildPipeline = !pipeline_ || !bindings_ ||
                            vertexShaderName_ != vertexShader || fragmentShaderName_ != fragmentShader ||
                            pipelineSampleCount_ != sampleCount;
                    if (rebuildPipeline) {
                        auto newBindings = std::unique_ptr<QRhiShaderResourceBindings>(context_->newShaderResourceBindings());
                        if (!newBindings) {
                            reportFailure("failed to allocate video shader bindings");
                        } else {
                            newBindings->setBindings(resources.begin(), resources.end());
                            if (!newBindings->create()) {
                                reportFailure("failed to create video shader bindings");
                            } else {
                                auto newPipeline = std::unique_ptr<QRhiGraphicsPipeline>(context_->newGraphicsPipeline());
                                const auto readShader = [](const QString& name) {
                                    QFile file(name);
                                    if (!file.open(QIODevice::ReadOnly))
                                        return QShader{};
                                    return QShader::fromSerialized(file.readAll());
                                };
                                const auto vertex = readShader(vertexShader);
                                const auto fragment = readShader(fragmentShader);
                                if (!newPipeline || !vertex.isValid() || !fragment.isValid()) {
                                    reportFailure("failed to load video shaders");
                                } else {
                                    newPipeline->setShaderStages({
                                            { QRhiShaderStage::Vertex, vertex },
                                            { QRhiShaderStage::Fragment, fragment }});
                                    QRhiVertexInputLayout input;
                                    input.setBindings({ { static_cast<quint32>(sizeof(float) * 4) } });
                                    input.setAttributes({
                                            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                            { 0, 1, QRhiVertexInputAttribute::Float2, static_cast<quint32>(sizeof(float) * 2) }});
                                    newPipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
                                    newPipeline->setSampleCount(sampleCount);
                                    newPipeline->setVertexInputLayout(input);
                                    newPipeline->setShaderResourceBindings(newBindings.get());
                                    newPipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
                                    if (!newPipeline->create()) {
                                        reportFailure("failed to create video pipeline");
                                    } else {
                                        bindings_ = std::move(newBindings);
                                        pipeline_ = std::move(newPipeline);
                                        vertexShaderName_ = vertexShader;
                                        fragmentShaderName_ = fragmentShader;
                                        pipelineSampleCount_ = sampleCount;
                                    }
                                }
                            }
                        }
                    } else {
                        bindings_->setBindings(resources.begin(), resources.end());
                        bindings_->updateResources();
                    }

                    if (!renderFailed_ && pipeline_ && bindings_) {
                        const QSize targetSize = renderTarget()->pixelSize();
                        const QSize frameSize = format.frameSize();
                        QRect sourceViewport = format.viewport();
                        if (!sourceViewport.isValid() || sourceViewport.isEmpty())
                            sourceViewport = QRect(QPoint{}, frameSize);
                        sourceViewport = sourceViewport.intersected(QRect(QPoint{}, frameSize));
                        if (targetSize.isEmpty() || sourceViewport.isEmpty()) {
                            reportFailure("video frame has no drawable area");
                        } else {
                            const auto transformation = qNormalizedFrameTransformation(frame);
                            const bool quarterTurn = qToUnderlying(transformation.rotation) % 180 != 0;
                            const float sourceWidth = static_cast<float>(sourceViewport.width());
                            const float sourceHeight = static_cast<float>(sourceViewport.height());
                            const float videoWidth = quarterTurn ? sourceHeight : sourceWidth;
                            const float videoHeight = quarterTurn ? sourceWidth : sourceHeight;
                            const float targetWidth = static_cast<float>(targetSize.width());
                            const float targetHeight = static_cast<float>(targetSize.height());
                            float widthScale = 1.f;
                            float heightScale = 1.f;
                            float cropLeft = 0.f;
                            float cropTop = 0.f;
                            float cropRight = 1.f;
                            float cropBottom = 1.f;
                            if (aspectRatioMode_ == Qt::KeepAspectRatio) {
                                const float scale = std::min(targetWidth / videoWidth, targetHeight / videoHeight);
                                widthScale = videoWidth * scale / targetWidth;
                                heightScale = videoHeight * scale / targetHeight;
                            } else if (aspectRatioMode_ == Qt::KeepAspectRatioByExpanding) {
                                const float videoAspect = videoWidth / videoHeight;
                                const float targetAspect = targetWidth / targetHeight;
                                if (videoAspect > targetAspect) {
                                    const float visible = targetAspect / videoAspect;
                                    cropLeft = (1.f - visible) / 2.f;
                                    cropRight = 1.f - cropLeft;
                                } else if (videoAspect < targetAspect) {
                                    const float visible = videoAspect / targetAspect;
                                    cropTop = (1.f - visible) / 2.f;
                                    cropBottom = 1.f - cropTop;
                                }
                            }

                            const float left = static_cast<float>(sourceViewport.left()) / static_cast<float>(frameSize.width());
                            const float top = static_cast<float>(sourceViewport.top()) / static_cast<float>(frameSize.height());
                            const float right = static_cast<float>(sourceViewport.right() + 1) / static_cast<float>(frameSize.width());
                            const float bottom = static_cast<float>(sourceViewport.bottom() + 1) / static_cast<float>(frameSize.height());
                            const auto sourceUv = [=](float x, float y) {
                                if (transformation.mirroredHorizontallyAfterRotation)
                                    x = 1.f - x;
                                switch (qToUnderlying(transformation.rotation)) {
                                case 90:
                                    return QPointF(left + y * (right - left), bottom - x * (bottom - top));
                                case 180:
                                    return QPointF(right - x * (right - left), bottom - y * (bottom - top));
                                case 270:
                                    return QPointF(right - y * (right - left), top + x * (bottom - top));
                                default:
                                    return QPointF(left + x * (right - left), top + y * (bottom - top));
                                }
                            };
                            const auto tl = sourceUv(cropLeft, cropTop);
                            const auto bl = sourceUv(cropLeft, cropBottom);
                            const auto tr = sourceUv(cropRight, cropTop);
                            const auto br = sourceUv(cropRight, cropBottom);
                            const std::array<float, 16> vertexData{
                                    -1.f, 1.f, static_cast<float>(tl.x()), static_cast<float>(tl.y()),
                                    -1.f, -1.f, static_cast<float>(bl.x()), static_cast<float>(bl.y()),
                                    1.f, 1.f, static_cast<float>(tr.x()), static_cast<float>(tr.y()),
                                    1.f, -1.f, static_cast<float>(br.x()), static_cast<float>(br.y())};
                            updates->updateDynamicBuffer(vertices_.get(), 0,
                                                         static_cast<quint32>(sizeof(vertexData)), vertexData.data());
                            QMatrix4x4 transform = context_->clipSpaceCorrMatrix();
                            transform.scale(widthScale, heightScale);
                            QByteArray uniformData;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
                            QVideoTextureHelper::updateUniformData(&uniformData, context_, format, frame,
                                                                   transform, 1.f);
#else
                            QVideoTextureHelper::updateUniformData(&uniformData, format, frame, transform, 1.f);
#endif
                            if (uniformData.size() != static_cast<qsizetype>(sizeof(QVideoTextureHelper::UniformData))) {
                                reportFailure("failed to prepare video uniforms");
                            } else {
                                updates->updateDynamicBuffer(uniforms_.get(), 0,
                                                             static_cast<quint32>(uniformData.size()), uniformData.constData());
                                draw = true;
                            }
                        }
                    }
                }
            }
        }

        commandBuffer->beginPass(renderTarget(), Qt::black, { 1.f, 0 }, updates);
        if (draw) {
            commandBuffer->setGraphicsPipeline(pipeline_.get());
            const QSize targetSize = renderTarget()->pixelSize();
            commandBuffer->setViewport({ 0.f, 0.f, static_cast<float>(targetSize.width()),
                                         static_cast<float>(targetSize.height()) });
            commandBuffer->setShaderResources(bindings_.get());
            const QRhiCommandBuffer::VertexInput input{ vertices_.get(), 0 };
            commandBuffer->setVertexInput(0, 1, &input);
            commandBuffer->draw(4);
        }
        commandBuffer->endPass();
    }

private:
    void reportFailure(const char* message) {
        if (renderFailed_)
            return;
        qWarning("VideoSurface: %s", message);
        renderFailed_ = true;
        QMetaObject::invokeMethod(this, [this] { emit renderFailed(); }, Qt::QueuedConnection);
    }

    QVideoSink sink_;
    std::unique_ptr<QVideoFrameTexturePool> pool_ = std::make_unique<QVideoFrameTexturePool>();
    std::unique_ptr<QRhiBuffer> vertices_;
    std::unique_ptr<QRhiBuffer> uniforms_;
    std::unique_ptr<QRhiSampler> sampler_;
    std::unique_ptr<QRhiShaderResourceBindings> bindings_;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline_;
    QRhi* context_ = nullptr;
    Qt::AspectRatioMode aspectRatioMode_ = Qt::KeepAspectRatio;
    QString vertexShaderName_;
    QString fragmentShaderName_;
    int pipelineSampleCount_ = 0;
    bool renderFailed_ = false;
};

} // namespace shadcn::detail
