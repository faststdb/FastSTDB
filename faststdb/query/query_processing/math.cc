#include "faststdb/query/query_processing/math.h"

namespace faststdb {
namespace qp {

struct Sum {
  double operator () (double lhs, double rhs) const {
    return lhs + rhs;
  }

  double unit() const {
    return 0.0;
  }
};

struct Diff {
  double operator () (double lhs, double rhs) const {
    return lhs - rhs;
  }

  double unit() const {
    return 0.0;
  }
};

struct Mul {
  double operator () (double lhs, double rhs) const {
    return lhs * rhs;
  }

  double unit() const {
    return 1.0;
  }
};

struct Divide {
  double operator () (double lhs, double rhs) const {
    return lhs / rhs;
  }

  double unit() const {
    return 1.0;
  }
};

static QueryParserToken<MathOperation<Sum>> sum_token("sum");
static QueryParserToken<MathOperation<Diff>> diff_token("diff");
static QueryParserToken<MathOperation<Mul>> mul_token("multiply");
static QueryParserToken<MathOperation<Divide>> div_token("divide");

}  // namespace qp
}  // namespace faststdb
