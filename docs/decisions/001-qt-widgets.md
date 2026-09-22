# Decision 001: Qt Widgets with a Qt-free core

Status: accepted for the initial implementation.

The runtime uses Qt Widgets because the requested interface is written in native C++. Qt supplies input dispatch, text editing, layouts, object ownership and base accessibility interfaces. The port owns its component drawing and shared design values.

The Qt-free core can compile and test independently. It contains only theme, range and timing logic. There is no renderer abstraction until a second implementation establishes a concrete need.

The trade-off is dependence on Qt's ABI, platform integrations and licence terms. A native Qt widget does not automatically have the operating system's stock appearance or shadcn's exact behaviour. Visual and behavioural evidence remain necessary.
