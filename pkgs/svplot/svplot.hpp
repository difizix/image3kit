/*!
  \file
  \brief Create 2D XY plots in Scalable Vector Graphic (SVG) format.
  \details Provides @c svplot data and function to create plots,
  and @c svplot_series to allow data values to be added.

  Very many functions allow fine control of the appearance and
  layout of plots, data markers and lines.\n

  (Many items common to 1D and 2D use functions and classes in @c axis_plot_frame).

  \author Jacob Voytko & Paul A. Bristow,  Ali Q. Raeini (2019)
 */

// Copyright Jacob Voytko 2007
// Copyright Paul A. Bristow 2007, 2008, 2009, 2012, 2013, 2014, 2016
// Copyright Ali Q. Raeini 2018, 2019

// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SVPLOT_HPP
#define BOOST_SVPLOT_HPP


#include "globals.h"
#include "typses.h"

#include <svgraphic.hpp>
#include <svplot_axes.hpp>

#include <string>
#include <ostream>
#include <iterator>
#include <iostream> // for debug.

namespace svg {


/*!
\class svg::svplot_series
\brief Holds a series of 2D data values (points) to be plotted.

\details Data values are sorted into normal and 'at limits':
  NaN, infinity or too small or too large.\n\n
 Member functions allow control of data points markers and lines joining them,
 and their appearance, shape, color and size.\n
 Data points can include their value,
 and optionally uncertainty and optionally number of degrees of freedom.\n

 Each data series can have a title that can be shown on a legend box with identifying symbols.

 @c data should be sorted correctly before being passed here. Data clean up and sorting has been removed largely, for simplicity and modularity
*/
class svplot_series
{
 public:
//! \cond DETAIL
  std::vector<dbl2> series_; //!< Normal 'OK to plot' data values.


  std::string title_; //!< Title of data series (to show on legend using legend style).
  point_style marker_; //!< Data point marker like circle, square, bar, histogram...
  point_style marker2_; //!< Data point marker like circle, square, bar, histogram...
  line_style  line_style_; //!< Style (color, width, fill area) of line joining data points.



  varsORv<double> pointScale_; //!< size/error-bar of data point markers, empty vector if not assigned
  varsORv<double> pointScale2_; //!<  Error-bar of data point marker f, empty vector if not assigned
  varsORv<double> seriesClr_; //!< Color of data points, empty vector if not assigned
  //varsORv<dbl3> seriesRotation_;
  //varsORv<dbl3> seriesLighting_;
  svg_color (*clrinterf_)(const float& f, svg_color c0, svg_color c1);

//! \endcond

 public:
  //!    Constructor for a data series to plot
  //!    \tparam T an STL container: for example: @c multimap.
  //!    \param begin Starting iterator into container of data series begin() to start at the beginning.
  //!    \param end Ending iterator into container of data series, end() to finish with the last item.
  //!    \param title Title for the plot.
  template <typename T>
  svplot_series(
      T begin, // \param Begin of data series.
      T end, // \param End of data series.
      std::string title = "",//  \param @c std::string title Title of data series.
      point_shape   pshap = circlet,
      svg_color     clr = blank,  /// fil and line colour only
      // std::vector<int>  dash = std::vector<int>(),
      svg_color (*clrInterpf)(const float& f, svg_color c0, svg_color c1) = &rgbtween
   )
   : title_(title), //!< Title of a series of data values.
    marker_(clr, clr*0.1, 5, pshap), // Default point style (default fill same as line color).
    marker2_(clr, clr*0.3, 15, pshap), // Default point style (default fill same as line color).
    line_style_(blank, blank, 2, false),  // Default line style, no fill, width 2, no line_on, no bezier.
    pointScale_(1.), pointScale2_(0.),seriesClr_(1.),
    clrinterf_(clrInterpf)
  {
    if(int n =std::distance(begin,end)) series_.reserve(n);
    else  {
      std::cout<<"Error invalid data sent to svplot_series"<<std::endl;
      return; }

    for(T i = begin; i != end; ++i)
      series_.push_back(dbl2(*i));// data values for both x and y.
  }

  template <typename T1, typename T2>
  svplot_series(
    T1 xbegin,  T1 xend,// \param Begin and end of x data series.
    T2 ybegin,// \param Begin of y data series.
    std::string title = "",
    point_shape pshap = circlet,
    svg_color   clr = blank,  /// fil and line colour only
    // std::vector<int>  dash = std::vector<int>(),
    svg_color  (*clrInterpf)(const float& f, svg_color c0, svg_color c1) = &rgbtween
   ) :
    title_(title), //!< Title of a series of data values.
    marker_(clr, clr*0.1, 5, pshap), // Default point style (default fill same as line color).
    marker2_(clr, clr*0.3, 15, pshap), // Default point style (default fill same as line color).
    line_style_(clr, blank, 2, false), // Default line style, no fill, width 2, no line, no bezier.
    pointScale_(1.), pointScale2_(0.),  seriesClr_(1.),
    clrinterf_(clrInterpf)
  {
    line_style_.stroke_on(false);//but keep colour

    if(int n = std::distance(xbegin,xend)) series_.reserve(n);
    else  {
      std::cout<<"Error invalid data sent to svplot_series"<<std::endl;
      return;  }

    while(xbegin != xend)
      series_.push_back(dbl2(*(xbegin++), *(ybegin++)));
  }


  svplot_series& fill_color(const svg_color& col_) {//! Set data series point marker fill color.
     marker_.fill_ = col_;   return *this;  }

  svplot_series& fill_color2(const svg_color& col_) {//! Set data series point marker fill color.
     marker2_.fill_ = col_;   return *this;  }

  svplot_series& stroke_color(const svg_color& col_) {//! Set Data series point marker stroke color.
     marker_.stroke_ = col_;   return *this;  }

  svplot_series& stroke_color2(const svg_color& col_) {//! Set Data series point marker stroke color.
     marker2_.stroke_ = col_;   return *this;  }

  svplot_series& shape(point_shape shape_) {//! Set Data series point marker shape.
   marker_.shape_ = shape_;   return *this;  }

  svplot_series& marker_size(int size) {//! Set Data series point marker size.
    marker_.size(size);   return *this;  }

  svplot_series& line_color(const svg_color& col) {//! Set Data series line color.
    line_style_.stroke_color(col);   return *this;  }

  svplot_series& area_fill(const svg_color& col_)  { //! Set Data series area fill color.
    //! \note @c area_fill(false) will produce a @b blank color, and so NO FILL. @c area_fill(blank) will produce the default non-blank color (black?).
    line_style_.area_fill(col_); return *this;  }

  svplot_series& line_width(double wid_){//! Set data series line width. //! (Sets legend line width too).
     line_style_.width(wid_);   return *this;  }

  svplot_series& line_on(bool on){//! Set data series line connections on/off.
     line_style_.stroke_on(on);   return *this;  }

  svplot_series&  dasharray(ints dashes){
      line_style_.set_css("stroke-dasharray=\""+_s(dashes)+"\"");   return *this;  }

  svplot_series& bezier_curve(bool on_) {//! Set @c true to link data points using bezier curves.
     line_style_.bezier_curve_ = on_;   return *this;  }

  template <typename C>
  svplot_series& color_scale(const C& contnts){//! Set
     seriesClr_ = Vars<double>(contnts.begin(),contnts.end());   return *this;  }

  template <typename C>
  svplot_series& point_scale(const C& contnts){//! Set
     pointScale_ = Vars<double>(contnts.begin(),contnts.end());   return *this;  }

  svplot_series& color_scale(const varsORv<double>& contnts){//! Set
     seriesClr_ = contnts;   return *this;  }

  svplot_series& point_scale(const varsORv<double>& contnts){//! Set
     pointScale_ = contnts;   return *this;  }

  svplot_series& color_interpolation(svg_color(*func)(const float& f, svg_color c0, svg_color c1)) {//! Set
     clrinterf_ = func;   return *this;  }

  template <typename C>
  svplot_series& y_error_bars(const C& contnts){//! Set
    pointScale_ = Vars<double>(contnts.begin(),contnts.end());
    marker2_.shape(y_error_bar);    return *this;  }// .set_css("")  requires a separate marker definition;

  template <typename C>
  svplot_series& x_error_bars(const C& contnts){//! Set
     pointScale2_ = Vars<double>(contnts.begin(),contnts.end());    marker2_.shape(x_error_bar);    return *this;  }


}; // class svplot_series






class svgraphic;

/*!
\brief Provides svplot data and member functions to create plots.\n
   Very many functions allow very fine control of the
   appearance and layout of plots, data markers and lines.
\sa svplot_series that allows data values to be added.
\sa svg_1d_plot.hpp for 1-D version.
\details
   svplot allows us to store plot state locally.\n
   (We don't store it in svg because transforming the points after they are
   written to the document would be difficult. We store the Cartesian
   coordinates locally and transform them before we write them).\n
   (svplot inherits from axis_plot_frame.hpp containing functions common to 1 and 2-D).
*/
class svplot : public svchart  {
  friend class svplot_series;
  #define Self_ (*this)
 public: //temporary for experimental gil was private:

  //! \cond DETAIL
  //! Member data names conventionally end with _,  for example: border_margin_,
  //! and corresponding set & get accessor functions are named without _ suffix,
  //! for example: border_margin() & border_margin(int).
  svgraphic* imageContainer_; //!< Stored so as to avoid rewriting style information constantly.

  double x_scale_; //!< scale factor used by transform() to go from Cartesian to SVG coordinates.
  double y_scale_; //!< scale factor used by transform() to go from Cartesian to SVG coordinates.
  double x_shift_; //!< shift factor used by transform() to go from Cartesian to SVG coordinates.
  double y_shift_; //!< shift factor used by transform() to go from Cartesian to SVG coordinates.
  double text_margin_; //!< Marginal space around text items like title, text_margin_ * font_size to get distance in svg units.



  text_element title_info_; //!< Plot title text and style

  bool pl_window_on_; //!< true if to use a separate plot window (not the whole image).
  box_element pl_box_; //!< plot background for plot titles and axes etc
  box_element area_; //!< plot area where data series are drawn
  const double plot_margin = 10.; //!< Plot window margin to allow to rounding etc
  double pl_left_; //!<TODO move to pl_box_,  SVG X coordinate (pixels) of left side of plot window.
  double pl_right_; //!< SVG X coordinate of right side of plot window.
  double pl_top_; //!< SVG Y coordinate of top side of plot window.
  double pl_bottom_; //!< SVG Y coordinate of bottom side of plot window.



  bool         x_values_on_; //!< true if values of X data are shown (as 1.23).
  bool         y_values_on_; //!< true if values of Y data are shown (as 3.45).
  rotate_style y_data_label_rotation_; //!< Direction point Y value labels written (default horizontal).
  value_style  x_values_style_; //!< Data point X value marking, font, size etc.
  value_style  y_values_style_; //!< Data point Y value marking, font, size etc.


  bool          legend_on_; //!< true if to provide a legend box.
  box_element   legend_; //!< rectangular box style of legend width, color...
  text_style    legend_style_; //!< Style for legend title and text.
  text_element  legend_header_; //!< Legend box header or title (if any).
  legend_places legend_place_; //!< Place for any legend box.
  double        legend_width_; //!<TODO move to legend_,  Width of legend box (pixels).
  double        legend_height_; //!< Height of legend box (in pixels).
  double        legend_left_; //!< Left of legend box.  // Size of legend box is controlled by its contents, but helpful to store computed coordinates.
  double        legend_top_; //!< Top of legend box.// Both optionally set by legend_top_left.

  axis_line_style  x_ax_; //!< Style of X-axis.
  axis_line_style  y_ax_; //!< Style of Y-axis.

  text_element x_label_info_; //!< X-axis label text, for example: "length".//!< Style for tick labels on X-axis.
  text_element y_label_info_; //!< Y-axis label text, for example: "volume". //!< Style for tick labels on Y-axis.


  double x_tight_; //!< Tolerance used by autoscale to avoid extra ticks.
  double y_tight_; //!< Tolerance used by autoscale to avoid extra ticks.
  tick_style x_ticks_; //!< Style of X-axis tick marks and labels.
  tick_style y_ticks_; //!< Style of Y-axis tick marks and labels.


  bool   x_include_zero_; //!< true if autoscale to include zero.
  bool   y_include_zero_; //!< @c true if autoscale to include zero.


  bool autoscale_check_limits_; //!< true if to check autoscale values for infinity, NaN, max, min.
  double autoscale_plusminus_; //!< For uncertain values, allow for text_plusminus ellipses showing 67%, 95% and 99% confidence limits.\n



  std::vector<svplot_series> serieses_; //!< Store of several series of data points for transformation.
  int shift_auto_color_;
  int auto_color_group_size_;
  svg_color         auto_color() { return color_array[shift_auto_color_+(serieses_.size()/auto_color_group_size_)%20]; }
  std::vector<int>  auto_stroke_dash() { return std::vector<int>(); }
  point_shape       auto_point_shape() { return point_shape(1+serieses_.size()%8); }



public:

  //!  brief Default constructor providing all the very many default plot options, some of which use some or all of the style defaults.\n
  //!  All these settings can be changed by these chainable functions.\n @b Example:
  //!  \code
  //!svplot my_plot;
  //!my_plot.background_color(ghostwhite) // Whole image.
  //!  .legend_border_color(yellow) // Just the legend box.
  //!  .legend_background_color(lightyellow)  // Fill color of the legend box.
  //!  .pl_background_color(svg_color(white)) // Just the plot window
  //!  .pl_border_color(svg_color(green)) // The border rectangle color.
  //!  .pl_border_width(1) // Thin border (SVG units, default pixels).
  //!  .title_color(red); // Title of whole image.
  //!
  //! \endcode
  //! \n
  //! \sa Rationale for default plot options and style settings in documentation.


  svplot() = delete;
  svplot(double Dx, double Dy, double X0, double Y0)
  :
    svchart(Dx,Dy,X0,Y0), // X0,Y0,Dx,Dy: Default image size for 2-D. leave 2 pixels for border, each side
    x_scale_(1.), y_scale_(1.),    // Used to transform Cartesian to SVG.
    x_shift_(0.), y_shift_(0.),
    text_margin_(1.4), // for axis label text, as a multiplier of the font size. AQR:  TODO ADJUST
    title_info_(0, 0, "", text_style(20), center_align, horizontal),

    pl_window_on_(true),
    pl_box_(white, white, 1, 3), // Should allow a 'half line space' above and below the label text  in update_internals because user can change tick value label font size.
    area_(dark, transparent, 1, 3),

     x_values_on_(false), // If X values of data points are shown.
    y_values_on_(false), // If Y values of data points are shown.
    x_values_style_(horizontal, 3),
    y_values_style_(downward, 3),

    legend_on_(false),
    legend_(white, svg_color(255,255,255,180), 1, 2),
    legend_style_(18, default_font, "", ""), // 2nd "italic"?
    legend_header_(0, 0, "", legend_style_, center_align, horizontal),
    legend_place_(outside_right), // default but interacts with using pl_window.
    legend_left_(-1), legend_top_(-1), // Default top left of plot window.
    x_ax_(dark, 1),//, -10., +10.
    y_ax_(dark, 1),//, -10., +10.
    x_label_info_(0, 0, "", text_style(18), center_align, horizontal),
    y_label_info_(0, 0, "", text_style(18), center_align, upward),
    x_tight_(1e-6), // margin that a point can lie outside top and bottom tick.
    y_tight_(1e-6), //!< margin that a point can lie outside top and bottom tick without triggering another interval and tick .
    x_ticks_(text_style(16)),// so for other defaults see no_style.
    y_ticks_(text_style(16)),


    x_include_zero_(false), // If autoscaled, include zero on X-axis.
    y_include_zero_(false), // If autoscaled, include zero on Y-axis.

    autoscale_check_limits_(true), // Do check all value for limits, infinity, max, min, NaN.
    autoscale_plusminus_(3.), // Allow 3 uncertainty (standard deviation) for 99% confidence ellipse.

    shift_auto_color_(2), // start color from black (2) 0 (blank) and 1 (transparent/none) are not shown
    auto_color_group_size_(1) //
  {

        // Build the document tree by adding all children of the root node.        // WARNING: sync order with pl_doc_structure
    Self_.add_g_element().id("Background");    //g_BACKGROUND = 0, //! Must be zero to index array document_ids_[]     for the whole svg image.
    Self_.add_g_element().id("plotArea");      //g_AREA, //! the smaller plot window (if used).                        for the smaller plot window (if used).
    Self_.add_g_element().id("yMinorGrid");    //g_Y_MINOR_GRID,     //! Y minor grid.
    Self_.add_g_element().id("yMajorGrid");    //g_Y_MAJOR_GRID,     //! Y major grid.
    Self_.add_g_element().id("xMinorGrid");    //g_X_MINOR_GRID,     //! X minor grid.
    Self_.add_g_element().id("xMajorGrid");    //g_X_MAJOR_GRID,     //! X major grid.
    Self_.add_g_element().id("plotLines");     //g_DATA_LINES,    //! Lines joining data points.
    Self_.add_g_element().id("plotPoints");    //g_DATA_POINTS, //! Normal data point markers.
    Self_.add_g_element().id("limitPoints");   //g_LIMIT_POINTS,  'At limit or NaN' data point markers.
    Self_.add_g_element().id("legendBox");     //g_LEGEND_BOX, //! Legend box.
    Self_.add_g_element().id("legendPoints");  //g_LEGEND_POINTS, //! Legend  series point markers, circle, cross...  .
    Self_.add_g_element().id("legendText");    //g_LEGEND_TEXT,   Legend text describing each data series.
    Self_.add_g_element().id("title");         //g_TITLE,             //! Title of the whole plot.
    Self_.add_g_element().id("plotXValues");   //g_X_POINT_VALUES, //! X Data point value labels.
    Self_.add_g_element().id("plotYValues");   //g_Y_POINT_VALUES, //! Y Data point value labels.
    Self_.add_g_element().id("plotFunctions"); //g_FUNCTIONS, //! Lines and curves, often to show a fit to the data.
    Self_.add_g_element().id("plotNotes");     //g_NOTES,      //! Free text and shapes to annotate a plot.
    Self_.add_g_element().id("yAxis");         //g_Y_AXIS,        //! X axis line.
    Self_.add_g_element().id("xAxis");         //g_X_AXIS,         //! Y axis line.
    Self_.add_g_element().id("yMinorTicks");   //g_Y_MINOR_TICKS, //! Y minor ticks.
    Self_.add_g_element().id("xMinorTicks");   //g_X_MINOR_TICKS, //! X minor ticks
    Self_.add_g_element().id("yMajorTicks");   //g_Y_MAJOR_TICKS, //! Y major ticks.
    Self_.add_g_element().id("xMajorTicks");   //g_X_MAJOR_TICKS, //! X major ticks.
    Self_.add_g_element().id("xTicksValues");  //g_X_TICKS_VALUES, //! X-axis tick values labels, for example 2, 4, 6...
    Self_.add_g_element().id("yTicksValues");  //g_Y_TICKS_VALUES, //! Y-axis tick values labels, for example 2, 4, 6...
    Self_.add_g_element().id("yLabel");        //g_Y_LABEL,      //! Y axis text labels "length (cm)".
    Self_.add_g_element().id("xLabel");        //g_X_LABEL,    //! X axis text labels "height (m)".


        // Set other SVG color, stroke & width defaults for various child PLOT nodes.
        Self_.g(g_BACKGROUND).style().fill_color(pl_box_.fill_);
        Self_.g(g_BACKGROUND).style().stroke_color(pl_box_.stroke_);
        Self_.g(g_BACKGROUND).style().stroke_width(pl_box_.width_); //
        Self_.g(g_AREA).style().fill_color(area_.fill_);
        Self_.g(g_AREA).style().stroke_width(area_.width_).stroke_color(area_.stroke_);
        Self_.g(g_LIMIT_POINTS).style().stroke_color(lightslategray).fill_color(antiquewhite);
        Self_.g(g_X_AXIS).style().stroke_color(black).stroke_width(x_ax_.width());
        Self_.g(g_Y_AXIS).style().stroke_color(black).stroke_width(y_ax_.width());


        // Ticks
        if(x_ticks_.ticks_side())  {
          Self_.g(g_X_MAJOR_TICKS).style().stroke_width(x_ticks_.major_tick_width_).stroke_color(black);
          Self_.g(g_X_MINOR_TICKS).style().stroke_width(x_ticks_.minor_tick_width_).stroke_color(black);
        }
        if(x_ticks_.ticks_side())  {
          Self_.g(g_Y_MAJOR_TICKS).style().stroke_width(y_ticks_.major_tick_width_).stroke_color(black);
          Self_.g(g_Y_MINOR_TICKS).style().stroke_width(y_ticks_.minor_tick_width_).stroke_color(black);
        }

        // Grids.
        // Default color & width for grid, used or not. Might avoid empty grid stuff if this was only done if grid used?  TODO
        Self_.g(g_X_MAJOR_GRID).style().stroke_width(x_ticks_.major_grid_width_).stroke_color(svg_color(200, 220, 255));
        Self_.g(g_X_MINOR_GRID).style().stroke_width(x_ticks_.minor_grid_width_).stroke_color(svg_color(200, 220, 255));
        Self_.g(g_Y_MAJOR_GRID).style().stroke_width(y_ticks_.major_grid_width_).stroke_color(svg_color(200, 220, 255));
        Self_.g(g_Y_MINOR_GRID).style().stroke_width(y_ticks_.minor_grid_width_).stroke_color(svg_color(200, 220, 255));
        Self_.g(g_DATA_LINES).style().stroke_width(2); // default width. // Alter with plot.data_lines_width(4); will be overwritten though, so useless

    } // svplot() default constructor.


 public: // Declarations of member functions.


  axis_line_style& x_axis() { return x_ax_;   }     //! \return @c horizontal X-axis line style

  axis_line_style& y_axis() { return y_ax_;   }     //! \return @c true if vertical Y-axis line to be drawn.

  tick_style& x_ticks()     { return x_ticks_;   }  //! \return @c true if ticks are to be marked on the X-axis.

  tick_style& y_ticks()     { return y_ticks_;   }  //! \return @c true if ticks are to be marked on the Y-axis.




  template <typename T> class has_begin { // SFINAE test
    typedef char one;   typedef int two;
    template <typename C> static one test( decltype(&C::cbegin) );
    template <typename C> static two test(...);
    public:    enum { value = sizeof(test<T>(0)) == sizeof(char) };
  };
  template <typename T>  class is_ptr {   // SFINAE test
    typedef char one;   typedef int two;
    template <typename C> static one test( decltype(&C::operator*()) ) ;
    template <typename C> static two test(...);
    public:    enum { value = sizeof(test<T>(0)) == sizeof(char) };
  };

  // Versions of plot functions to add data series from a container, all or part,
  // declarations including defaults for parameters (except containers, of course).

  template <typename T1, typename T2, typename std::enable_if<has_begin<T1>::value,bool>::type = 0,
               typename std::enable_if<has_begin<T2>::value,bool>::type = 0>
  svplot_series& plot(const T1& container1, const T2& container2, const std::string& title = "")  {
    if(container1.size()!=container2.size()) std::cout<<"Error size mismatch:  "<<container1.size()<<"!="<<container2.size()<<std::endl;
    serieses_.push_back(
      svplot_series(
      container1.begin(),
      container1.end(),
      container2.begin(),
      title, auto_point_shape(), auto_color())//, auto_stroke_dash()
    );
    return serieses_[serieses_.size()-1]; //! \return Reference to data series just added to make chainable.
  }

  template <typename T>
  svplot_series& plot(const T& container, const std::string& title = "")  {
   //!  \brief Add a container of a data series to the plot.\n @b Example:
   //! \code
  //! my_plot.plot(data1, "Sqrt(x)");
   //! \endcode
   //! \note This version assumes that @b ALL the data values in the container is used.
   //!  \tparam T Type of data in series (must be compatible with std::pair).
    serieses_.push_back(
      svplot_series(  container.begin(),  container.end(),
      title, auto_point_shape(), auto_color())//, auto_stroke_dash()
    );
    return serieses_[serieses_.size()-1]; //! \return Reference to data series just added to make chainable.
  }

  template <typename T, typename std::enable_if<svplot::is_ptr<T>::value,bool>::type = 0>
  svplot_series& plot(const T& begin, const T& end, const std::string& title = "") {
  //!  \brief Add a data series to the plot (by default, converting automatically to @c unc doubles).\n
  //!  This version permits @b part of the container to be used, a partial range, using iterators begin to end.\n
  //!  For example:
  //!  \code
  //!   my_2d_plot.plot(my_data.begin(), my_data.end(), "My container");
  //!  \endcode
  //!  \code
  //!   my_2d_plot.plot(&my_data[1], &my_data[3], "my_data 1 to 3"); // Add part of data series.
  //! \endcode
    serieses_.push_back(
     svplot_series( begin,  end, title, auto_point_shape(), auto_color())//, auto_stroke_dash()
    );

    return serieses_[serieses_.size() - 1]; //! \return Reference to data series  to make chainable.
  }




  #include <svplot_draw.hpp>
  #include <svplot_axis_x.hpp>
  #include <svplot_axis_y.hpp>

  #include <svplot_chain_set.hpp>




 }; // class svplot

} // namespace svg
#endif // BOOST_SVPLOT_HPP
