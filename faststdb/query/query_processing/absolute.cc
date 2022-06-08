/*!
 * \file absolute.cc
 */
#include "faststdb/query/query_processing/absolute.h"

namespace faststdb {
namespace qp {

Absolute::Absolute(std::shared_ptr<Node> next)
    : next_(next)
{
}

Absolute::Absolute(const boost::property_tree::ptree&, ReshapeRequest const&, std::shared_ptr<Node> next)
: next_(next)
{
}

void Absolute::complete() {
  next_->complete();
}

bool Absolute::put(MutableSample &mut) {
  auto size = mut.size();
  for (u32 ix = 0; ix < size; ix++) {
    double* value = mut[ix];
    if (value) {
      *value = std::abs(*value);
    }
  }
  return next_->put(mut);
}

void Absolute::set_error(common::Status status) {
  next_->set_error(status);
}

int Absolute::get_requirements() const {
  return TERMINAL;
}

static QueryParserToken<Absolute> token("abs");

}  // namespace qp
}  // namespace faststdb
