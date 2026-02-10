#include "stmt.h"
using namespace xel;

std::any Expression::accept(StmtVisitor* visitor) {
    return visitor->visit_expression_stmt(shared_from_this());
}

std::any Print::accept(StmtVisitor* visitor) {
    return visitor->visit_print_stmt(shared_from_this());
}

std::any Var::accept(StmtVisitor* visitor) {
    return visitor->visit_var_stmt(shared_from_this());
}

std::any Block::accept(StmtVisitor* visitor) {
    return visitor->visit_block_stmt(shared_from_this());
}

std::any If::accept(StmtVisitor* visitor) {
    return visitor->visit_if_stmt(shared_from_this());
}

std::any While::accept(StmtVisitor* visitor) {
    return visitor->visit_while_stmt(shared_from_this());
}

std::any Function::accept(StmtVisitor* visitor) {
    return visitor->visit_function_stmt(shared_from_this());
}

std::any Return::accept(StmtVisitor* visitor) {
    return visitor->visit_return_stmt(shared_from_this());
}