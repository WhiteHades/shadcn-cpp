// SPDX-License-Identifier: MIT
#include "demos.hpp"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <array>
#include <shadcn/shadcn.hpp>

namespace gallery {
using namespace shadcn;
QStringList components() {
    QStringList names{"alert-dialog",  "badge",        "button",     "card",         "checkbox",
                      "dialog",        "drawer",       "hover-card", "input",        "label",
                      "popover",       "progress",     "separator",  "sheet",        "skeleton",
                      "switch",        "tooltip",      "alert",      "aspect-ratio", "avatar",
                      "breadcrumb",    "button-group", "empty",      "input-group",  "item",
                      "kbd",           "radio-group",  "slider",     "spinner",      "textarea",
                      "toggle",        "toggle-group", "calendar",   "chart",        "combobox",
                      "command",       "date-picker",  "field",      "form",         "input-otp",
                      "native-select", "select",       "table",      "data-table",
                      "accordion", "collapsible", "tabs", "scroll-area", "resizable",
                      "sidebar", "navigation-menu", "menubar", "dropdown-menu", "context-menu",
                      "carousel", "pagination", "direction", "toast", "sonner", "attachment",
                      "bubble", "message", "message-scroller", "questionnaire", "marker"};
    names.sort();
    return names;
}
QWidget* demo(const QString& name, QWidget* parent) {
    auto* canvas = new QWidget(parent);
    canvas->setAutoFillBackground(true);
    auto* outer = new QHBoxLayout(canvas);
    outer->setContentsMargins(32, 32, 32, 32);
    auto* host = new QWidget(canvas);
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);
    outer->addStretch();
    outer->addWidget(host, 0, Qt::AlignCenter);
    outer->addStretch();
    if (name == "button") {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        auto* button = new Button("Button", host);
        button->setVariant(Variant::Outline);
        row->addWidget(button);
        auto* icon = new Button({}, host);
        icon->setButtonSize(ButtonSize::IconSm);
        icon->setVariant(Variant::Outline);
        icon->setAccessibleName("Move up");
        QPixmap image(32, 32);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(host->palette().color(QPalette::WindowText), 2.7, Qt::SolidLine, Qt::RoundCap,
                      Qt::RoundJoin));
        p.drawLine(16, 24, 16, 8);
        p.drawLine(16, 8, 8, 16);
        p.drawLine(16, 8, 24, 16);
        p.end();
        icon->setIcon(QIcon(image));
        row->addWidget(icon);
        layout->addLayout(row);
    } else if (name == "card") {
        auto* card = new Card(host);
        card->setTitle("Create project");
        card->setDescription("Start a new project in one click.");
        card->setFixedWidth(350);
        auto* label = new Label("Name", card);
        auto* input = new Input(card);
        input->setPlaceholderText("Project name");
        label->setBuddy(input);
        card->content().addWidget(label);
        card->content().addWidget(input);
        auto* cancel = new Button("Cancel", card);
        cancel->setVariant(Variant::Outline);
        card->footer().addWidget(cancel);
        card->footer().addStretch();
        card->footer().addWidget(new Button("Create", card));
        layout->addWidget(card);
    } else if (name == "input") {
        auto* input = new Input(host);
        input->setPlaceholderText("Email");
        input->setAccessibleName("Email");
        input->setFixedWidth(320);
        layout->addWidget(input);
    } else if (name == "badge") {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        const std::array variants{Variant::Default, Variant::Secondary, Variant::Outline,
                                  Variant::Destructive};
        const std::array labels{"Badge", "Secondary", "Outline", "Destructive"};
        for (std::size_t i = 0; i < variants.size(); ++i) {
            auto* b = new Badge(labels[i], host);
            b->setVariant(variants[i]);
            row->addWidget(b);
        }
        layout->addLayout(row);
    } else if (name == "checkbox") {
        auto* check = new Checkbox("Accept terms and conditions", host);
        check->setChecked(true);
        layout->addWidget(check);
    } else if (name == "label") {
        auto* label = new Label("Email address", host);
        auto* input = new Input(host);
        input->setPlaceholderText("you@example.com");
        input->setFixedWidth(320);
        label->setBuddy(input);
        layout->addWidget(label);
        layout->addWidget(input);
    } else if (name == "switch") {
        auto* row = new QHBoxLayout;
        row->setSpacing(8);
        auto* control = new Switch(host);
        auto* label = new Label("Airplane mode", host);
        label->setBuddy(control);
        row->addWidget(control);
        row->addWidget(label);
        layout->addLayout(row);
    } else if (name == "separator") {
        host->setFixedWidth(280);
        auto* title = new Label("shadcn/ui", host);
        layout->addWidget(title);
        layout->addWidget(new QLabel("Components for your next application.", host));
        layout->addWidget(new Separator(Qt::Horizontal, host));
        auto* row = new QHBoxLayout;
        row->addWidget(new QLabel("Blog", host));
        row->addWidget(new Separator(Qt::Vertical, host));
        row->addWidget(new QLabel("Docs", host));
        row->addWidget(new Separator(Qt::Vertical, host));
        row->addWidget(new QLabel("Source", host));
        layout->addLayout(row);
    } else if (name == "progress") {
        auto* progress = new Progress(host);
        progress->setAccessibleName("Download progress");
        progress->setFixedWidth(320);
        progress->setValue(60);
        layout->addWidget(progress);
    } else if (name == "skeleton") {
        host->setFixedWidth(280);
        auto* first = new Skeleton(host);
        first->setFixedHeight(32);
        layout->addWidget(first);
        auto* second = new Skeleton(host);
        second->setFixedWidth(200);
        layout->addWidget(second);
        layout->addWidget(new Skeleton(host));
    } else if (name == "dialog" || name == "alert-dialog" || name == "sheet" || name == "drawer") {
        Dialog* dialog = name == "alert-dialog" ? static_cast<Dialog*>(new AlertDialog(canvas))
                         : name == "sheet"  ? static_cast<Dialog*>(new Sheet(Side::Right, canvas))
                         : name == "drawer" ? static_cast<Dialog*>(new Drawer(canvas))
                                            : new Dialog(canvas);
        dialog->setTitle(name == "alert-dialog" ? "Are you sure?" : "Edit profile");
        dialog->setDescription(name == "alert-dialog"
                                   ? "This action cannot be undone."
                                   : "Make changes to your profile here. Save when you are done.");
        if (name != "alert-dialog") {
            auto* label = new Label("Name", &dialog->panel());
            auto* input = new Input(&dialog->panel());
            input->setText("Alex Morgan");
            label->setBuddy(input);
            dialog->content().addWidget(label);
            dialog->content().addWidget(input);
            auto* save = new Button("Save changes", &dialog->panel());
            dialog->footer().addWidget(save);
            QObject::connect(save, &Button::clicked, dialog, &Dialog::accept);
        }
        auto* button = new Button(name == "alert-dialog" ? "Delete account" : "Edit profile", host);
        button->setVariant(Variant::Outline);
        layout->addWidget(button);
        QObject::connect(button, &Button::clicked, dialog, &Dialog::open);
    } else if (name == "popover" || name == "hover-card" || name == "tooltip") {
        auto* button = new Button(name == "hover-card" ? "@shadcn"
                                  : name == "tooltip"  ? "Hover"
                                                       : "Open popover",
                                  host);
        button->setVariant(Variant::Outline);
        layout->addWidget(button);
        if (name == "tooltip") {
            auto* tip = new Tooltip("Add to library", canvas);
            tip->attach(*button);
            canvas->setProperty("capturePopup", QVariant::fromValue<QObject*>(tip));
            canvas->setProperty("captureAnchor", QVariant::fromValue<QObject*>(button));
        } else {
            Popover* popup = name == "hover-card" ? static_cast<Popover*>(new HoverCard(canvas))
                                                  : new Popover(canvas);
            canvas->setProperty("capturePopup", QVariant::fromValue<QObject*>(popup));
            canvas->setProperty("captureAnchor", QVariant::fromValue<QObject*>(button));
            auto* title = new Label(name == "hover-card" ? "shadcn/ui" : "Dimensions", popup);
            popup->content().addWidget(title);
            auto* description =
                new QLabel(name == "hover-card" ? "Components for building interfaces."
                                                : "Set the dimensions for the layer.",
                           popup);
            description->setWordWrap(true);
            popup->content().addWidget(description);
            if (name == "hover-card")
                static_cast<HoverCard*>(popup)->attach(*button);
            else
                QObject::connect(button, &Button::clicked, popup,
                                 [popup, button] { popup->showFor(*button); });
        }
    } else if (name == "alert") {
        auto* alert = new Alert(host);
        alert->setFixedWidth(380);
        alert->setTitle("Your changes have been saved");
        alert->setDescription("You can continue working on your project.");
        layout->addWidget(alert);
    } else if (name == "avatar") {
        auto* row = new QHBoxLayout;
        for (const auto& initials : {"CN", "AM", "JD"})
            row->addWidget(new Avatar(initials, host));
        layout->addLayout(row);
    } else if (name == "aspect-ratio") {
        auto* aspect = new AspectRatio(16.0 / 9.0, host);
        aspect->setFixedWidth(320);
        auto* label = new QLabel("16 : 9", aspect);
        label->setAlignment(Qt::AlignCenter);
        label->setAutoFillBackground(true);
        auto palette = label->palette();
        palette.setColor(QPalette::Window, palette.color(QPalette::AlternateBase));
        label->setPalette(palette);
        aspect->setWidget(*label);
        layout->addWidget(aspect);
    } else if (name == "breadcrumb") {
        auto* trail = new Breadcrumb(host);
        trail->addLink("Home");
        trail->addSeparator();
        trail->addLink("Components");
        trail->addSeparator();
        trail->addPage("Breadcrumb");
        layout->addWidget(trail);
    } else if (name == "button-group") {
        auto* group = new ButtonGroup(Qt::Horizontal, host);
        for (const auto* text : {"Previous", "1", "2", "3", "Next"}) {
            auto* button = new Button(text, group);
            button->setVariant(Variant::Outline);
            group->addButton(*button);
        }
        layout->addWidget(group);
    } else if (name == "input-group") {
        auto* group = new InputGroup(host);
        group->setFixedWidth(340);
        group->addText("https://");
        auto* input = new Input(group);
        input->setPlaceholderText("example.com");
        input->setAccessibleName("Website");
        group->addInput(*input);
        group->addButton("Go");
        layout->addWidget(group);
    } else if (name == "empty") {
        auto* empty = new Empty(host);
        empty->setFixedWidth(380);
        empty->setTitle("No projects yet");
        empty->setDescription("Create your first project to get started.");
        empty->addWidget(*new Button("Create project", empty));
        layout->addWidget(empty);
    } else if (name == "item") {
        auto* item = new Item(host);
        item->setFixedWidth(400);
        item->setVariant(ItemVariant::Outline);
        item->setTitle("Alex Morgan");
        item->setDescription("alex@example.com");
        item->addLeading(*new Avatar("AM", item));
        auto* button = new Button("Follow", item);
        button->setVariant(Variant::Outline);
        item->addTrailing(*button);
        layout->addWidget(item);
    } else if (name == "kbd") {
        auto* row = new QHBoxLayout;
        row->addWidget(new QLabel("Press", host));
        auto* group = new KbdGroup(host);
        group->addKey("Ctrl");
        group->addKey("K");
        row->addWidget(group);
        row->addWidget(new QLabel("to search", host));
        layout->addLayout(row);
    } else if (name == "radio-group") {
        auto* group = new RadioGroup(host);
        for (const auto* text : {"Default", "Comfortable", "Compact"})
            group->addItem(*new RadioGroupItem(text, group), text);
        group->setCheckedValue("Comfortable");
        layout->addWidget(group);
    } else if (name == "slider") {
        auto* slider = new Slider(host);
        slider->setFixedWidth(320);
        slider->setValues({25, 75});
        slider->setAccessibleName("Price range");
        layout->addWidget(slider);
    } else if (name == "spinner") {
        auto* row = new QHBoxLayout;
        row->addWidget(new Spinner(host));
        row->addWidget(new QLabel("Loading...", host));
        layout->addLayout(row);
    } else if (name == "textarea") {
        auto* text = new Textarea(host);
        text->setFixedSize(340, 100);
        text->setPlaceholderText("Type your message here.");
        text->setAccessibleName("Message");
        layout->addWidget(text);
    } else if (name == "toggle") {
        auto* toggle = new Toggle("B", host);
        toggle->setVariant(ToggleVariant::Outline);
        toggle->setAccessibleName("Bold");
        auto font = toggle->font();
        font.setBold(true);
        toggle->setFont(font);
        toggle->setChecked(true);
        layout->addWidget(toggle);
    } else if (name == "toggle-group") {
        auto* group = new ToggleGroup(Qt::Horizontal, host);
        group->setMode(ToggleGroupMode::Multiple);
        for (const auto* text : {"Bold", "Italic", "Underline"}) {
            auto* toggle = new Toggle(text, group);
            toggle->setVariant(ToggleVariant::Outline);
            group->addToggle(*toggle, text);
        }
        group->setCheckedValues({"Bold"});
        layout->addWidget(group);
    } else if (name == "calendar") {
        auto* calendar = new Calendar(host);
        calendar->setSelectedDate(QDate(2026, 9, 22));
        layout->addWidget(calendar);
    } else if (name == "date-picker") {
        auto* picker = new DatePicker(host);
        picker->setDate(QDate(2026, 9, 22));
        layout->addWidget(picker);
    } else if (name == "select" || name == "native-select" || name == "combobox") {
        NativeSelect* select = name == "combobox" ? static_cast<NativeSelect*>(new Combobox(host))
                               : name == "select" ? static_cast<NativeSelect*>(new Select(host))
                                                  : new NativeSelect(host);
        select->setFixedWidth(240);
        select->addItems({"Apple", "Banana", "Blueberry", "Grapes", "Pineapple"});
        select->setCurrentIndex(-1);
        select->setPlaceholderText("Select a fruit");
        select->setAccessibleName("Fruit");
        if (auto* combo = qobject_cast<Combobox*>(select))
            combo->setPlaceholderText("Search fruit...");
        layout->addWidget(select);
    } else if (name == "command") {
        auto* command = new Command(host);
        command->setFixedWidth(340);
        command->addGroup("Suggestions");
        command->addItem("Calendar");
        command->addItem("Search files");
        command->addItem("Settings");
        layout->addWidget(command);
    } else if (name == "field") {
        auto* field = new Field("Email", host);
        field->setFixedWidth(340);
        auto* input = new Input(field);
        input->setPlaceholderText("you@example.com");
        field->setControl(input);
        field->setDescription("We will use this address to contact you.");
        layout->addWidget(field);
    } else if (name == "form") {
        auto* form = new Form(host);
        form->setFixedWidth(340);
        auto* input = new Input(form);
        input->setPlaceholderText("alex");
        auto& field = form->addField("Username", input);
        field.setDescription("This is your public display name.");
        field.setValidator([input] {
            return input->text().size() < 2 ? QString("Use at least 2 characters.") : QString{};
        });
        auto* submit = new Button("Submit", form);
        form->content().addWidget(submit, 0, Qt::AlignLeft);
        QObject::connect(submit, &Button::clicked, form, &Form::submit);
        layout->addWidget(form);
    } else if (name == "input-otp") {
        auto* code = new InputOTP(6, host);
        code->setCode("123");
        layout->addWidget(code);
        layout->addWidget(new QLabel("Enter your verification code.", host));
    } else if (name == "table" || name == "data-table") {
        Table* table =
            name == "data-table" ? static_cast<Table*>(new DataTable(host)) : new Table(host);
        table->setFixedSize(440, 185);
        auto* model = new QStandardItemModel(table);
        model->setHorizontalHeaderLabels({"Invoice", "Status", "Amount"});
        model->appendRow(
            {new QStandardItem("INV001"), new QStandardItem("Paid"), new QStandardItem("$250.00")});
        model->appendRow({new QStandardItem("INV002"), new QStandardItem("Pending"),
                          new QStandardItem("$150.00")});
        model->appendRow(
            {new QStandardItem("INV003"), new QStandardItem("Paid"), new QStandardItem("$350.00")});
        if (auto* data = qobject_cast<DataTable*>(table)) {
            data->setSourceModel(model);
            auto* filter = new Input(host);
            filter->setPlaceholderText("Filter invoices...");
            QObject::connect(filter, &Input::textChanged, data,
                             [data](const QString& text) { data->setFilter(text); });
            layout->addWidget(filter);
        } else
            table->setModel(model);
        layout->addWidget(table);
    } else if (name == "accordion") {
        auto* accordion = new Accordion(host);
        accordion->setFixedWidth(400);
        auto& first = accordion->addItem("Is it accessible?");
        auto* answer = new QLabel("Use the arrow keys to move between sections.", &first);
        answer->setWordWrap(true);
        first.content().addWidget(answer);
        first.setExpanded(true);
        accordion->addItem("Is it styled?").content().addWidget(new QLabel("The theme follows shadcn/ui."));
        accordion->addItem("Is it animated?").content().addWidget(new QLabel("Motion respects your application settings."));
        layout->addWidget(accordion);
    } else if (name == "collapsible") {
        auto* section = new Collapsible("3 repositories", host);
        section->setFixedWidth(340);
        for (const auto* text : {"@shadcn/ui", "@shadcn/ui/templates", "@shadcn/ui/blocks"})
            section->content().addWidget(new QLabel(text, section));
        section->setOpen(true);
        layout->addWidget(section);
    } else if (name == "tabs") {
        auto* tabs = new Tabs(Qt::Horizontal, host);
        tabs->setFixedWidth(360);
        for (const auto* nameValue : {"Account", "Password"}) {
            (void)tabs->addTab(nameValue, nameValue);
            auto* card = new Card(tabs);
            card->setTitle(nameValue);
            card->setDescription("Make changes to your account here.");
            auto* input = new Input(card);
            input->setPlaceholderText(nameValue == QString("Account") ? "Name" : "Current password");
            card->content().addWidget(input);
            card->footer().addWidget(new Button("Save changes", card));
            tabs->addContent(nameValue, *card);
        }
        layout->addWidget(tabs);
    } else if (name == "scroll-area") {
        auto* scroll = new ScrollArea(host);
        scroll->setFixedSize(280, 210);
        auto* contents = new QWidget;
        auto* rows = new QVBoxLayout(contents);
        rows->addWidget(new Label("Tags", contents));
        for (int i = 1; i <= 30; ++i) {
            rows->addWidget(new QLabel(QString("v1.2.0-beta.%1").arg(i), contents));
            rows->addWidget(new Separator(Qt::Horizontal, contents));
        }
        scroll->setWidget(contents);
        layout->addWidget(scroll);
    } else if (name == "resizable") {
        auto* panels = new ResizablePanelGroup(Qt::Horizontal, host);
        panels->setFixedSize(400, 190);
        for (const auto* text : {"One", "Two", "Three"}) {
            auto* panel = new Card(panels);
            auto* label = new Label(text, panel);
            label->setAlignment(Qt::AlignCenter);
            panel->content().addWidget(label);
            panels->addPanel(*panel);
        }
        panels->setPanelSizes({140, 120, 140});
        layout->addWidget(panels);
    } else if (name == "sidebar") {
        auto* provider = new SidebarProvider(host);
        provider->setFixedSize(500, 240);
        auto* sidebar = new Sidebar(provider);
        sidebar->setExpandedWidth(180);
        sidebar->header().addWidget(new Label("Workspace", sidebar));
        (void)sidebar->addMenuButton("Home", true);
        (void)sidebar->addMenuButton("Inbox");
        (void)sidebar->addMenuButton("Calendar");
        (void)sidebar->addMenuButton("Settings");
        provider->addSidebar(*sidebar);
        provider->content().addWidget(new SidebarTrigger(provider), 0, Qt::AlignLeft);
        provider->content().addWidget(new Label("Your workspace", provider));
        provider->content().addStretch();
        layout->addWidget(provider);
    } else if (name == "navigation-menu") {
        auto* navigation = new NavigationMenu(host);
        (void)navigation->addLink("Home");
        (void)navigation->addMenu("Components", "components");
        auto* contents = new QWidget(navigation);
        auto* links = new QVBoxLayout(contents);
        links->addWidget(new Label("Components", contents));
        links->addWidget(new QLabel("Buttons, inputs and layouts.", contents));
        navigation->setMenuContent("components", *contents);
        (void)navigation->addLink("Documentation");
        layout->addWidget(navigation);
    } else if (name == "menubar") {
        auto* menus = new Menubar(host);
        for (const auto* title : {"File", "Edit", "View", "Profile"}) {
            auto* menu = menus->addMenu(title);
            menu->addAction("New tab");
            menu->addAction("New window");
            menu->addSeparator();
            menu->addAction("Close");
        }
        layout->addWidget(menus);
    } else if (name == "dropdown-menu") {
        auto* button = new Button("Open menu", host);
        button->setVariant(Variant::Outline);
        auto* menu = new DropdownMenu(button);
        menu->addLabel("My account");
        menu->addSeparatorLine();
        menu->addItem("Profile");
        menu->addItem("Settings");
        menu->addCheckboxItem("Notifications", true);
        button->setMenu(menu);
        layout->addWidget(button);
    } else if (name == "context-menu") {
        auto* target = new Card(host);
        target->setFixedSize(300, 180);
        auto* text = new QLabel("Right click here", target);
        text->setAlignment(Qt::AlignCenter);
        target->content().addWidget(text);
        auto* menu = new ContextMenu(target);
        menu->addItem("Back");
        menu->addItem("Forward");
        menu->addItem("Reload");
        menu->attach(*target);
        layout->addWidget(target);
    } else if (name == "carousel") {
        auto* carousel = new Carousel(Qt::Horizontal, host);
        carousel->setFixedSize(350, 210);
        for (int i = 1; i <= 5; ++i) {
            auto* card = new Card(carousel);
            auto* number = new Label(QString::number(i), card);
            auto font = number->font(); font.setPixelSize(30); number->setFont(font);
            number->setAlignment(Qt::AlignCenter);
            card->content().addWidget(number);
            carousel->addSlide(*card);
        }
        layout->addWidget(carousel);
    } else if (name == "pagination") {
        auto* pages = new Pagination(host);
        pages->setPageCount(10);
        pages->setCurrentPage(3);
        layout->addWidget(pages);
    } else if (name == "direction") {
        auto* direction = new DirectionProvider(Direction::RightToLeft, host);
        auto* row = new QHBoxLayout;
        row->addWidget(new Button("First", direction));
        row->addWidget(new Button("Second", direction));
        row->addWidget(new Button("Third", direction));
        direction->content().addLayout(row);
        direction->content().addWidget(new QLabel("Right to left layout", direction));
        layout->addWidget(direction);
    } else if (name == "toast") {
        auto* toast = new Toast(1, "Event has been created", "Tuesday, September 22 at 9:00 AM", host);
        toast->setDuration(0);
        toast->setActionText("Undo");
        toast->setFixedWidth(380);
        layout->addWidget(toast);
    } else if (name == "sonner") {
        auto* notifications = new Sonner(host);
        notifications->setFixedWidth(380);
        notifications->addToast("Changes saved", "Your settings are up to date.", ToastType::Success, 0);
        layout->addWidget(notifications);
        auto* button = new Button("Show notification", host);
        button->setVariant(Variant::Outline);
        QObject::connect(button, &Button::clicked, notifications, [notifications] {
            notifications->showToast("Event has been created", "You can undo this change.", ToastType::Default, 4000, "Undo");
        });
        layout->addWidget(button, 0, Qt::AlignCenter);
    } else if (name == "attachment") {
        auto* attachment = new Attachment(host);
        attachment->setFixedWidth(340);
        attachment->setTitle("Design brief.pdf");
        attachment->setDescription("PDF document · 2.4 MB");
        (void)attachment->addAction("Open");
        layout->addWidget(attachment);
    } else if (name == "bubble") {
        host->setFixedWidth(380);
        auto* question = new Bubble("How can I get started?", host);
        question->setVariant(BubbleVariant::Secondary);
        question->setAlign(BubbleAlign::End);
        layout->addWidget(question);
        layout->addWidget(new Bubble("Add a component and make it your own.", host));
    } else if (name == "message") {
        auto* message = new Message(host);
        message->setFixedWidth(380);
        auto* avatar = new Avatar("AM", message);
        message->setAvatar(*avatar);
        message->addHeader(*new Label("Alex Morgan", message));
        message->setText("The design looks good. Let's build it.");
        message->addFooter(*new QLabel("10:42 AM", message));
        layout->addWidget(message);
    } else if (name == "message-scroller") {
        auto* scroller = new MessageScroller(host);
        scroller->setFixedSize(400, 220);
        for (const auto* text : {"Hi Alex, is the project ready?", "Yes. The components are in place.", "Can you send me a preview?", "Here it is. Let me know what you think."}) {
            auto* message = new Message(scroller);
            message->setText(text);
            scroller->addWidget(*message);
        }
        layout->addWidget(scroller);
    } else if (name == "questionnaire") {
        auto* questions = new Questionnaire(host);
        questions->setFixedWidth(420);
        const int index = questions->addQuestion("What would you like to build?", "Choose a starting point.");
        questions->addChoice(index, "desktop", "Desktop application", "A native interface for your users.");
        questions->addChoice(index, "tool", "Developer tool", "A focused tool for your workflow.");
        questions->setAnswer(index, {"desktop"});
        layout->addWidget(questions);
    } else if (name == "marker") {
        host->setFixedWidth(360);
        auto* marker = new Marker("Today", host);
        marker->setVariant(MarkerVariant::Separator);
        layout->addWidget(marker);
    } else if (name == "chart") {
        auto* chart = new Chart(host);
        chart->setFixedSize(480, 240);
        chart->setLabels({"Jan", "Feb", "Mar", "Apr", "May", "Jun"});
        (void)chart->setSeries({{"Desktop", {186, 305, 237, 273, 209, 214}, Role::Chart1},
                                {"Mobile", {80, 200, 120, 190, 130, 140}, Role::Chart3}});
        layout->addWidget(chart);
    }
    return canvas;
}

QPixmap capture(QWidget& canvas) {
    QWidget* overlay = nullptr;
    if (auto* dialog = canvas.findChild<Dialog*>(QString{}, Qt::FindDirectChildrenOnly)) {
        dialog->open();
        overlay = dialog;
    } else if (auto* popup = qobject_cast<Popover*>(canvas.property("capturePopup").value<QObject*>())) {
        auto* anchor = qobject_cast<QWidget*>(canvas.property("captureAnchor").value<QObject*>());
        if (anchor) {
            popup->showFor(*anchor, qobject_cast<Tooltip*>(popup) ? Side::Top : Side::Bottom);
            overlay = popup;
        }
    }
    QApplication::processEvents();
    auto image = canvas.grab();
    if (overlay) {
        QPainter painter(&image);
        painter.drawPixmap(canvas.mapFromGlobal(overlay->mapToGlobal(QPoint{})), overlay->grab());
        painter.end();
        overlay->hide();
    }
    return image;
}
} // namespace gallery
