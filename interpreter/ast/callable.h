#pragma once

#include "../evaluator/evaluator.h"
#include <ctime>

namespace xel
{
    class Callable
    {
        public:
            virtual int arity() = 0;
            virtual std::any call(Evaluator* evaluator, std::vector<std::any> arguments) = 0;
            virtual std::string to_string() {return "callable";}
    };

    // function
    class function : public Callable
    {
        public:
            function(std::shared_ptr<Function> declaration) :_declaration(declaration) {}
            std::any call(Evaluator* evaluator, std::vector<std::any> arguments) override{
                Environment* environment = new Environment(evaluator->_globals);
                for(int i=0; i<_declaration->_params.size(); i++)
                    environment->define(_declaration->_params[i].get_lexeme(), arguments[i]);
                evaluator->execute_block(_declaration->_body, environment);
                return nullptr;
            }
            int arity() override { return _declaration->_params.size(); }
            std::string to_string() override { return "<fn " + _declaration->_name.get_lexeme() + ">"; }
        private:
            std::shared_ptr<Function> _declaration;
    };


    // clock
    class clock : public Callable
    {
        public:
            int arity() override { return 0; }
            std::any call(Evaluator* evaluator, std::vector<std::any> arguments) override {
                return static_cast<double>(std::time(nullptr));
            }
            std::string to_string() override { return "<native fn>"; }
    };
}