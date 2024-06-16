#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "apriltag_utils.h"  // Include your apriltag header
#include "opencv2/opencv.hpp"

namespace py = pybind11;

// Wrapper function
apriltag_family_t* setup_tag_family_wrapper(const std::string& famname) {
  apriltag_family_t* tf = nullptr;
  setup_tag_family(&tf, famname.c_str());
  return tf;
}

apriltag_detector_t* create_detector(apriltag_family_t* tf) {
  apriltag_detector_t* td = apriltag_detector_create();
  apriltag_detector_add_family(td, tf);
  td->quad_decimate = 2.0;
  td->quad_sigma = 0.0;
  td->nthreads = 1;
  td->debug = false;
  td->refine_edges = true;
  td->wp = workerpool_create(1);
  return td;
}

zarray* detect_tags_cpu(apriltag_detector_t* td, cv::Mat& im) {
  if (im.channels() != 1 || im.depth() != CV_8U) {
    throw std::invalid_argument("Image must be grayscale and 8-bit");
  }
  image_u8_t image = {im.cols, im.rows, im.cols, im.data};
  zarray_t* detections = apriltag_detector_detect(td, &image);

  return detections;
}

PYBIND11_MODULE(apriltag_bindings, m) {
  m.def("setup_tag_family", &setup_tag_family_wrapper,
        py::return_value_policy::reference, "Setup the tag family",
        py::arg("famname"));

  m.def("create_detector", &create_detector, py::return_value_policy::reference,
        "Create a detector and add a tag family", py::arg("tf"));

  m.def("detect_tags_cpu", &detect_tags_cpu, py::return_value_policy::reference,
        "Detect tags in an opencv Mat image", py::arg("tf"), py::arg("im"));

  py::class_<apriltag_family>(m, "AprilTagFamily")
      .def(py::init<>())  // Default constructor
      .def_readwrite("ncodes", &apriltag_family::ncodes)
      .def_readwrite("codes", &apriltag_family::codes)
      .def_readwrite("width_at_border", &apriltag_family::width_at_border)
      .def_readwrite("total_width", &apriltag_family::total_width)
      .def_readwrite("reversed_border", &apriltag_family::reversed_border)
      .def_readwrite("nbits", &apriltag_family::nbits)
      .def_readwrite("bit_x", &apriltag_family::bit_x)
      .def_readwrite("bit_y", &apriltag_family::bit_y)
      .def_readwrite("h", &apriltag_family::h)
      .def_readwrite("name", &apriltag_family::name)
      .def_readwrite("impl", &apriltag_family::impl);

  py::class_<apriltag_detector>(m, "AprilTagDetector")
      .def(py::init<>())  // Default constructor
      .def_readwrite("nthreads", &apriltag_detector::nthreads)
      .def_readwrite("quad_decimate", &apriltag_detector::quad_decimate)
      .def_readwrite("quad_sigma", &apriltag_detector::quad_sigma)
      .def_readwrite("refine_edges", &apriltag_detector::refine_edges)
      .def_readwrite("decode_sharpening", &apriltag_detector::decode_sharpening)
      .def_readwrite("debug", &apriltag_detector::debug)
      .def_readwrite("qtp", &apriltag_detector::qtp)
      .def_readwrite("tp", &apriltag_detector::tp)
      .def_readwrite("nedges", &apriltag_detector::nedges)
      .def_readwrite("nsegments", &apriltag_detector::nsegments)
      .def_readwrite("nquads", &apriltag_detector::nquads)
      .def_readwrite("tag_families", &apriltag_detector::tag_families)
      .def_readwrite("wp", &apriltag_detector::wp)
      .def_readwrite("mutex", &apriltag_detector::mutex);

  /*py::class_<apriltag_detection>(m, "AprilTagDetection")
      .def(py::init<>())  // Default constructor
      .def_readwrite("family", &apriltag_detection::family)
      .def_readwrite("id", &apriltag_detection::id)
      .def_readwrite("hamming", &apriltag_detection::hamming)
      .def_readwrite("decision_margin", &apriltag_detection::decision_margin)
      .def_readwrite("H", &apriltag_detection::H)
      .def_readwrite("c", &apriltag_detection::c)
      .def_readwrite("p", &apriltag_detection::p);*/

  py::class_<zarray>(m, "zarray")
      .def(py::init<>())  // Default constructor
      .def_readwrite("el_sz", &zarray::el_sz)
      .def_readwrite("size", &zarray::size)
      .def_readwrite("alloc", &zarray::alloc)
      .def_readwrite("data", &zarray::data);

  m.def("zarray_size", &zarray_size, "Get the size of the zarray",
        py::arg("za"));
}
