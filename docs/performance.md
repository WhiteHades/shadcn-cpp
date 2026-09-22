# Performance

This snapshot makes no claim to be the fastest C++ UI library. The current measurements cover the Qt-free core build and tests. There are no native rendering, allocation, binary-size or application-startup measurements yet.

A theme stores 31 colour roles in a fixed array. Lookup and colour replacement take constant time and constant extra space. The colour converter evaluates a fixed set of arithmetic operations and allocates no memory. Progress normalisation also takes constant time and constant extra space.

The timing evaluator uses 32 bounded bisection steps to invert the cubic curve. Its cost is constant for this implementation. The current widgets reuse their QVariantAnimation objects instead of allocating one per frame. Skeleton animations stop while hidden. These are implementation properties, not comparative benchmark results.

The initial widgets target groups the small controls in one source file. Fine-grained component source selection and linker-size measurements remain distribution work; the current API does not promise per-component binary pruning.

Text measurement and shaping depend on text length and the platform font engine. Painting cost also depends on the widget's pixel area, display scale and graphics backend. A constant-time style lookup does not make an entire render constant-time.

For later lists and data tables, use Qt's model/view controls and measure view cost against visible rows. Do not create a QWidget for every row of a large data set. Sorting has an expected comparison-sort cost of O(n log n); filtering must inspect the candidate records unless a suitable index changes the query. The eventual API must state its data-size assumptions.

Before a performance claim, record the commit, compiler, build type, Qt version, platform backend, resolution, display scale and test data. Measure idle wakeups, frame-time percentiles, peak resident memory, allocations, startup time and package size. Keep the benchmark code and raw results. Compare against a defined alternative under the same conditions.
