#pragma once

#include "../evaluator/evaluator.h"
#include "../xel_error.h"
#include <ctime>
#include <chrono>

namespace xel
{
    class Callable
    {
        public:
            virtual int arity() = 0;
            virtual var call(Evaluator* evaluator, std::vector<var> arguments) = 0;
            virtual std::string to_string() {return "callable";}
    };

    // function
    class function : public Callable
    {
        public:
            function(std::shared_ptr<Function> declaration, Environment* closure) :_declaration(declaration), _closure(closure) {}
            var call(Evaluator* evaluator, std::vector<var> arguments) override{
                Environment* environment = new Environment(_closure);
                for(int i=0; i<_declaration->_params.size(); i++)
                    environment->define(_declaration->_params[i].get_lexeme(), arguments[i]);

                try {
                    evaluator->execute_block(_declaration->_body, environment);
                } catch ( xel::error::Return return_value ) {
                    // TODO : optimize
                    return return_value.value;
                }

                return nullptr;
            }
            int arity() override { return _declaration->_params.size(); }
            std::string to_string() override { return "<fn " + _declaration->_name.get_lexeme() + ">"; }
        private:
            std::shared_ptr<Function> _declaration;
            Environment* _closure;
    };


    // clock
    class clock : public Callable
    {
        public:
            int arity() override { return 0; }
            var call(Evaluator* evaluator, std::vector<var> arguments) override {
                auto now = std::chrono::system_clock::now().time_since_epoch();
                return duration_cast<std::chrono::microseconds>(now).count() / 1000.0;
            }
            std::string to_string() override { return "<native fn>"; }
    };
}