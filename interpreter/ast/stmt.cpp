#include "stmt.h"
using namespace xel;

var Expression::accept(StmtVisitor* visitor) {
    return visitor->visit_expression_stmt(shared_from_this());
}

var Print::accept(StmtVisitor* visitor) {
    return visitor->visit_print_stmt(shared_from_this());
}

var Var::accept(StmtVisitor* visitor) {
    return visitor->visit_var_stmt(shared_from_this());
}

var Block::accept(StmtVisitor* visitor) {
    return visitor->visit_block_stmt(shared_from_this());
}

var If::accept(StmtVisitor* visitor) {
    return visitor->visit_if_stmt(shared_from_this());
}

var While::accept(StmtVisitor* visitor) {
    return visitor->visit_while_stmt(shared_from_this());
}

var Function::accept(StmtVisitor* visitor) {
    return visitor->visit_function_stmt(shared_from_this());
}

var Return::accept(StmtVisitor* visitor) {
    return visitor->visit_return_stmt(shared_from_this());
}