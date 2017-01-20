# Shadows

This is a D3D11 sample app that demonstrates several techniques for rendering real-time shadow maps. The following techniques are implemented:

* Cascaded Shadow Maps
* Stabilized Cascaded Shadow Maps
* Automatic Cascade Fitting based on depth buffer analysis, as in [Sample Distribution Shadow Maps](https://software.intel.com/en-us/articles/sample-distribution-shadow-maps).
* Various forms of Percentage Closer Filtering
* [Variance Shadow Maps](http://www.punkuser.net/vsm/vsm_paper.pdf)
* [Exponential Variance Shadow Maps](http://www.punkuser.net/lvsm/lvsm_web.pdf) (EVSM)
* [Moment Shadow Maps](http://cg.cs.uni-bonn.de/en/publications/paper-details/peters-2015-msm/)

This code sample was done as part of 2 articles for my blog, which you can find here:

* [https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/](https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/)
* [https://mynameismjp.wordpress.com/2015/02/18/shadow-sample-update/](https://mynameismjp.wordpress.com/2015/02/18/shadow-sample-update/)

# Build Instructions

The repository contains a Visual Studio 2015 project and solution file that's ready to build and run on Windows 7, 8, or 10. All external dependencies are included in the repository.
