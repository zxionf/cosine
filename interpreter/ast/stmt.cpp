#include "stmt.h"
using namespace xel;

std::any Expression::accept(StmtVisitor* visitor) {
    return visitor->visit_expression_stmt(shared_from_this());
}

std::any Print::accept(StmtVisitor* visitor) {
    return visitor->visit_print_stmt(shared_from_this());
}