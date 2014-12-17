#include <vector>

#include "caffe/common.hpp"
#include "caffe/layer.hpp"
#include "caffe/vision_layers.hpp"
#include "caffe/syncedmem.hpp"
#include <sstream>

namespace caffe {

template <typename Dtype>
void MatReadLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom,
				      const vector<Blob<Dtype>*>& top) {
  iter_ = 0;
  prefix_ = this->layer_param_.mat_read_param().prefix();
  period_ = this->layer_param_.mat_read_param().period();
  CHECK_GT(period_, 0) << "period must be positive";
  if (this->layer_param_.mat_read_param().has_source()) {
    std::ifstream infile(this->layer_param_.mat_read_param().source().c_str());
    CHECK(infile.good()) << "Failed to open source file "
			 << this->layer_param_.mat_read_param().source();
    const int strip = this->layer_param_.mat_read_param().strip();
    CHECK_GE(strip, 0) << "Strip cannot be negative";
    string linestr;
    while (std::getline(infile, linestr)) {
      std::istringstream iss(linestr);
      string filename;
      iss >> filename;
      CHECK_GT(filename.size(), strip) << "Too much stripping";
      fnames_.push_back(filename.substr(0, filename.size() - strip));
    }
    LOG(INFO) << "MatRead will load a maximum of " << fnames_.size() << " files.";
  }
}

template <typename Dtype>
void MatReadLayer<Dtype>::Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
}

template <typename Dtype>
void MatReadLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    const vector<Blob<Dtype>*>& top) {
  if (iter_ % period_ == 0) {
    for (int i = 0; i < bottom.size(); ++i) {
      std::ostringstream oss;
      oss << prefix_;
      if (this->layer_param_.mat_read_param().has_source()) {
	CHECK_LT(iter_, fnames_.size()) << "Test has run for more iterations than it was supposed to";
	oss << fnames_[iter_];
      }
      else {
	oss << "iter_" << iter_;
      }
      oss << "_blob_" << i << ".mat";
      top[i]->FromMat(oss.str().c_str());
    }
  }
  ++iter_;
}

template <typename Dtype>
void MatReadLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const vector<bool>& propagate_down,
    const vector<Blob<Dtype>*>& bottom) {
  return;
}

INSTANTIATE_CLASS(MatReadLayer);
REGISTER_LAYER_CLASS(MAT_READ, MatReadLayer);

}  // namespace caffe
