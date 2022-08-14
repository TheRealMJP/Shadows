# Shadows

This is a D3D11 sample app that demonstrates several techniques for rendering real-time shadow maps. The following techniques are implemented:

* Cascaded Shadow Maps
* Stabilized Cascaded Shadow Maps
* Automatic Cascade Fitting based on depth buffer analysis, as in [Sample Distribution Shadow Maps](https://software.intel.com/en-us/articles/sample-distribution-shadow-maps).
* Various forms of Percentage Closer Filtering
* [Variance Shadow Maps](https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.104.2569&rep=rep1&type=pdf)
* [Exponential Variance Shadow Maps](https://dl.acm.org/doi/pdf/10.5555/1375714.1375739) (EVSM)
* [Moment Shadow Maps](https://momentsingraphics.de/I3D2015.html)

This code sample was done as part of 2 articles for my blog, which you can find here:

* [https://therealmjp.github.io/posts/shadow-maps/](https://therealmjp.github.io/posts/shadow-maps/)
* [https://therealmjp.github.io/posts/shadow-sample-update/](https://therealmjp.github.io/posts/shadow-sample-update/)

# Build Instructions

The repository contains a Visual Studio 2015 project and solution file that's ready to build and run on Windows 7, 8, or 10. All external dependencies are included in the repository.
