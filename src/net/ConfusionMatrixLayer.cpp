/*
 * This file is part of the CN24 semantic segmentation software,
 * copyright (C) 2015 Clemens-Alexander Brust (ikosa dot de at gmail dot com).
 *
 * For licensing information, see the LICENSE file included with this project.
 */  

#include <string>
#include <iomanip>
#include <sstream>
#include "ConfusionMatrixLayer.h"

namespace Conv {
ConfusionMatrixLayer::ConfusionMatrixLayer (
  std::vector<std::string>& names, const unsigned int classes) :
  classes_ (classes), names_(names) {
  LOGDEBUG << "Instance created, " << classes << " classes.";
}

bool ConfusionMatrixLayer::CreateOutputs (
  const std::vector< CombinedTensor* >& inputs,
  std::vector< CombinedTensor* >& outputs) {
  // Validate input node count
  if (inputs.size() != 3) {
    LOGERROR << "Need exactly 3 inputs to calculate binary stat!";
    return false;
  }

  CombinedTensor* first = inputs[0];
  CombinedTensor* second = inputs[1];
  CombinedTensor* third = inputs[2];

  // Check for null pointers
  if (first == nullptr || second == nullptr || third == nullptr) {
    LOGERROR << "Null pointer node supplied";
    return false;
  }

  if (first->data.samples() != second->data.samples()) {
    LOGERROR << "Inputs need the same number of samples!";
    return false;
  }

  if (second->data.elements() != second->data.samples()) {
    LOGERROR << "This layer needs the NUMBER of the class";
    return false;
  }

  if (first->data.samples() != third->data.samples()) {
    LOGERROR << "Inputs need the same number of samples!";
    return false;
  }

  // Needs no outputs
  return true;
}

bool ConfusionMatrixLayer::Connect
(const std::vector< CombinedTensor* >& inputs,
 const std::vector< CombinedTensor* >& outputs) {
  // Needs exactly three inputs to calculate the stat
  if (inputs.size() != 3)
    return false;

  // Also, the two inputs have to have the same number of samples and elements!
  // We ignore the shape for now...
  CombinedTensor* first = inputs[0];
  CombinedTensor* second = inputs[1];
  CombinedTensor* third = inputs[2];
  bool valid = first != nullptr && second != nullptr &&
               first->data.samples() == second->data.samples() &&
               first->data.samples() == third->data.samples() &&
               outputs.size() == 0;

  if (valid) {
    first_ = first;
    second_ = second;
    third_ = third;

    matrix_ = new long double[classes_ * classes_];
    per_class_ = new long double[classes_];
    
    Reset();
  }

  return valid;
}

void ConfusionMatrixLayer::FeedForward() {
  if (disabled_)
    return;

  for (unsigned int sample = 0; sample < first_->data.samples(); sample++) {
    unsigned int first_class = first_->data.Maximum (sample);
    const duint second_class = * ( (const duint*) second_->data.data_ptr_const (sample));
    const long double weight = third_->data (sample);
    matrix_[(first_class * classes_) + second_class] += weight;
    per_class_[second_class] += weight;
    total_ += weight;
    if(first_class == second_class)
      right_ += weight;
  }
}

void ConfusionMatrixLayer::BackPropagate() {

}


void ConfusionMatrixLayer::Reset() {
  for (unsigned int c = 0; c < (classes_ * classes_); c++) {
    matrix_[c] = 0;
  }
  for (unsigned int c = 0; c < classes_; c++) {
    per_class_[c] = 0;
  }
  total_ = 0;
  right_ = 0;
}

void ConfusionMatrixLayer::Print (std::string prefix, bool training) {
  std::stringstream caption;
  caption << std::setw (12) << "vCLS  ACT>";
  for (unsigned int c = 0; c < classes_; c++) {
    caption << std::setw (12) << names_[c];
  }
  LOGRESULT << caption.str() << LOGRESULTEND;
  caption.str ("");


  for (unsigned int t = 0; t < classes_; t++) {
    caption << std::setw (12) << names_[t];
    for (unsigned int c = 0; c < classes_; c++) {
      long double result = matrix_[ (t * classes_) + c];
      caption << std::setw (12) << static_cast<long>(result);
    }

    LOGRESULT << caption.str() << LOGRESULTEND;
    caption.str ("");
  }
  

  LOGRESULT << prefix << " Overall recognition rate (not normalized): "
    << 100.0L * right_ / total_ << "%";
    
  long double ccount = 0;
  long double sum = 0;
  for (unsigned int c = 0; c < classes_; c++) {
    if(per_class_[c] > 0) {
      sum += matrix_[(c * classes_) + c] / per_class_[c];
      ccount += 1.0L;
    }
  }
  
  LOGRESULT << prefix << " Average recognition rate (normalized)    : "
    << 100.0 * sum / ccount << "%" << LOGRESULTEND;


}

void ConfusionMatrixLayer::PrintCSV (std::ostream& output) {
  // Output CSV formatted data
  std::stringstream csv_caption;
  csv_caption << "classified,";
  for(unsigned int c = 0; c < classes_; c++) {
    csv_caption << names_[c];
    if((c+1) < classes_)
      csv_caption << ",";
  }

  output << csv_caption.str() << std::endl;
  std::stringstream csv_line;
  for(unsigned int t = 0; t < classes_; t++) {
    csv_line << names_[t] << ",";
    for (unsigned int c = 0; c < classes_; c++) {
      long double result = matrix_[ (t * classes_) + c];
      csv_line << static_cast<long>(result);
      if((c+1) < classes_)
        csv_line << ",";
    }
    output << csv_line.str() << std::endl;
    csv_line.str("");
  }
}

ConfusionMatrixLayer::~ConfusionMatrixLayer() {
  if (matrix_ != nullptr)
    delete[] matrix_;
}




}