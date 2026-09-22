# Ownership and safety

The library uses C++23 value types for themes, strong enums for variants and `std::expected` for recoverable value errors. Widget ownership follows Qt's parent tree. QApplication owns the installed style, widgets own their animations, and signal connections use a receiver context.

`make_child` constructs a parent-owned widget and returns a borrowed reference. The helper constrains the template to a constructible QWidget subclass. It removes a repeated allocation-and-parenting step; it does not add a borrow checker.

```cpp
auto& button = shadcn::make_child<shadcn::Button>(window, "Continue");
QPointer<shadcn::Button> observed(&button);
QObject::connect(&timer, &QTimer::timeout, &window, [observed] {
    if (observed) observed->setEnabled(true);
});
```

Include `QTimer` and `QPointer` in the application using this snippet. The receiver context prevents the callback from running after the window is destroyed. QPointer becomes null when its QObject is destroyed. Neither makes concurrent widget access safe. Keep all widget operations on the GUI thread.

Use stack objects for root widgets. Use Qt parenting for children. Use `std::unique_ptr` for a genuinely independent root or non-QObject resource when it is the sole owner. Never give a parent-owned widget a second owning smart pointer. Do not reparent an object whose stack lifetime conflicts with the new parent's lifetime.

Validation rejects invalid finite ranges, NaN and infinity at the core's public interfaces. An invalid theme update leaves the old value intact. Qt widgets retain Qt's inherited contracts, including QProgressBar's treatment of out-of-range integer values.

C++ does not provide Rust's compile-time lifetime and aliasing checks. These ownership rules reduce common mistakes, but misuse remains possible. AddressSanitizer and UndefinedBehaviorSanitizer are configured for supported toolchains. Their successful core run does not prove safety of the unbuilt Qt library, Qt itself or an application that uses it.

Qt's [object ownership documentation](https://doc.qt.io/qt-6/objecttrees.html) explains the parent tree and stack-lifetime rules. [QObject documentation](https://doc.qt.io/qt-6/qobject.html) defines connection and thread-affinity behaviour.
