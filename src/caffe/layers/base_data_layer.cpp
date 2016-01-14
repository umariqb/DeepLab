#include <string>
#include <vector>

#include "caffe/data_layers.hpp"
#include "caffe/util/io.hpp"

namespace caffe {

template <typename Dtype>
BaseDataLayer<Dtype>::BaseDataLayer(const LayerParameter& param)
    : Layer<Dtype>(param),
      transform_param_(param.transform_param()),
      data_transformer_(transform_param_) {
}

template <typename Dtype>
void BaseDataLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  if (top.size() == 1) {
    output_labels_ = false;
  } else if (top.size() >= 2){  // jay change
    output_labels_ = true;
  }
  // The subclasses should setup the size of bottom and top
  DataLayerSetUp(bottom, top);
  data_transformer_.InitRand();
}

template <typename Dtype>
void BasePrefetchingDataLayer<Dtype>::LayerSetUp(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  BaseDataLayer<Dtype>::LayerSetUp(bottom, top);
  // Now, start the prefetch thread. Before calling prefetch, we make two
  // cpu_data calls so that the prefetch thread does not accidentally make
  // simultaneous cudaMalloc calls when the main thread is running. In some
  // GPUs this seems to cause failures if we do not so.
  this->prefetch_data_.mutable_cpu_data();
  if (this->output_labels_) {
    this->prefetch_label_.mutable_cpu_data();
  }
  DLOG(INFO) << "Initializing prefetch";
  this->CreatePrefetchThread();
  DLOG(INFO) << "Prefetch initialized.";
}

template <typename Dtype>
void BasePrefetchingDataLayer<Dtype>::CreatePrefetchThread() {
  this->phase_ = Caffe::phase();
  this->data_transformer_.InitRand();
  CHECK(StartInternalThread()) << "Thread execution failed";
}

template <typename Dtype>
void BasePrefetchingDataLayer<Dtype>::JoinPrefetchThread() {
  CHECK(WaitForInternalThreadToExit()) << "Thread joining failed";
}

template <typename Dtype>
void BasePrefetchingDataLayer<Dtype>::Forward_cpu(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  // First, join the thread
  JoinPrefetchThread();
  DLOG(INFO) << "Thread joined";
  // Copy the data
  caffe_copy(prefetch_data_.count(), prefetch_data_.cpu_data(),
             top[0]->mutable_cpu_data());
  DLOG(INFO) << "Prefetch copied";
  if (this->output_labels_) {
    caffe_copy(prefetch_label_.count(), prefetch_label_.cpu_data(),
               top[1]->mutable_cpu_data());
  }
  // Start a new prefetch thread
  DLOG(INFO) << "CreatePrefetchThread";
  CreatePrefetchThread();
}

  /* 
   * Jay add
   *
   */
template <typename Dtype>
void ImageDimPrefetchingDataLayer<Dtype>::LayerSetUp(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  BaseDataLayer<Dtype>::LayerSetUp(bottom, top);
  if (top.size() == 3) {
    output_data_dim_ = true;
  } else {
    output_data_dim_ = false;
  }
  // Now, start the prefetch thread. Before calling prefetch, we make two
  // cpu_data calls so that the prefetch thread does not accidentally make
  // simultaneous cudaMalloc calls when the main thread is running. In some
  // GPUs this seems to cause failures if we do not so.
  this->prefetch_data_.mutable_cpu_data();
  if (this->output_labels_) {
    this->prefetch_label_.mutable_cpu_data();
  }
  if (output_data_dim_) {
    prefetch_data_dim_.mutable_cpu_data();
  }

  DLOG(INFO) << "Initializing prefetch";
  BasePrefetchingDataLayer<Dtype>::CreatePrefetchThread();
  DLOG(INFO) << "Prefetch initialized.";
}

  /* 
   * Umar add
   *
   */
template <typename Dtype>
void ImageDimPrefetchingDataLayer<Dtype>::Reshape(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
   // reshape data
   int N = this->prefetch_data_.num();
   int K = this->prefetch_data_.channels();
   int H = this->prefetch_data_.height();
   int W = this->prefetch_data_.width();
   top[0]->Reshape(N, K, H, W);

   // reshape label
   N = this->prefetch_label_.num();
   K = this->prefetch_label_.channels();
   H = this->prefetch_label_.height();
   W = this->prefetch_label_.width();
   top[1]->Reshape(N, K, H, W);
}


template <typename Dtype>
void ImageDimPrefetchingDataLayer<Dtype>::Forward_cpu(
    const vector<Blob<Dtype>*>& bottom, const vector<Blob<Dtype>*>& top) {
  // First, join the thread
  BasePrefetchingDataLayer<Dtype>::JoinPrefetchThread();
  DLOG(INFO) << "Thread joined";
  // Copy the data
  CHECK_LE(this->prefetch_data_.count(), top[0]->count());
  caffe_copy(this->prefetch_data_.count(), this->prefetch_data_.cpu_data(),
             top[0]->mutable_cpu_data());
  DLOG(INFO) << "Prefetch copied";
  if (this->output_labels_) {
  CHECK_LE(this->prefetch_label_.count(), top[1]->count());
    caffe_copy(this->prefetch_label_.count(), this->prefetch_label_.cpu_data(),
               top[1]->mutable_cpu_data());
  }
  if (output_data_dim_) {
  CHECK_LE(this->prefetch_data_dim_.count(), top[2]->count());   
  caffe_copy(prefetch_data_dim_.count(), prefetch_data_dim_.cpu_data(),
	       top[2]->mutable_cpu_data());
  }

  // Start a new prefetch thread
  DLOG(INFO) << "CreatePrefetchThread";
  BasePrefetchingDataLayer<Dtype>::CreatePrefetchThread();
}


#ifdef CPU_ONLY
STUB_GPU_FORWARD(BasePrefetchingDataLayer, Forward);
STUB_GPU_FORWARD(ImageDimPrefetchingDataLayer, Forward);
#endif

INSTANTIATE_CLASS(BaseDataLayer);
INSTANTIATE_CLASS(BasePrefetchingDataLayer);
INSTANTIATE_CLASS(ImageDimPrefetchingDataLayer);
}  // namespace caffe
