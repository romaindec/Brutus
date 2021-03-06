// ---------------------------------------------------------- C++ System Headers
#include <iostream>
#include <string>
#include <vector>

// ------------------------------------------------------------- Project Headers
#include "CProgAST.h"
#include "IR.h"

////////////////////////////////////////////////////////////////////////////////
// class CProgAST                                                             //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor

CProgASTProgram::~CProgASTProgram()
{
    for(CProgASTFuncdef* funcdef : funcdefs)
    {
        delete funcdef;
    }
}

// ----------------------------------------------------- Public Member Functions
void CProgASTProgram::add_funcdef(CProgASTFuncdef* funcdef)
{
    funcdefs.push_back(funcdef);
}

void CProgASTProgram::build_ir(IR& ir) const
{
    for(CProgASTFuncdef* funcdef : funcdefs)
    {
        ir.add_cfg(funcdef->build_ir(&ir.global_symbols));
    }
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTFuncdef                                                      //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTFuncdef::CProgASTFuncdef(std::string id, Type type) :
    identifier(id), return_type(type)
{}

CProgASTFuncdef::~CProgASTFuncdef()
{
    for(CProgASTStatement* statement : statements)
    {
        delete statement;
    }
}

// ----------------------------------------------------- Public Member Functions
void CProgASTFuncdef::add_arg(std::string id, Type type)
{
    arg_types.push_back(type);
    arg_names.push_back(id);
}

void CProgASTFuncdef::add_statement(CProgASTStatement* statement)
{
    statements.push_back(statement);
}

CFG* CProgASTFuncdef::build_ir(TableOfSymbols* global_symbols) const
{
    global_symbols->add_symbol(identifier, return_type);
    SymbolProperties& fproperties = global_symbols->get_symbol(identifier);
    fproperties.callable = true;
    fproperties.arg_types = arg_types;

    CFG* cfg = new CFG(this, identifier, global_symbols);
    for(size_t i=0; i<arg_names.size(); ++i)
    {
        cfg->add_arg_to_symbol_table(arg_names[i], arg_types[i]);
    }
    for(CProgASTStatement* statement : statements)
    {
        statement->build_ir(cfg);
    }
    return cfg;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTReturn : public CProgASTStatement                            //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTReturn::CProgASTReturn(CProgASTExpression* expression) :
    return_expression(expression)
{}

CProgASTReturn::~CProgASTReturn()
{
    delete return_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTReturn::build_ir(CFG* cfg) const
{
    std::string rval = return_expression->build_ir(cfg);
    cfg->current_bb->add_IRInstr(IRInstr::ret, cfg->get_var_type(rval), {rval});
    return ""; // ??
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTDeclaration : public CProgASTStatement                       //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTDeclaration::CProgASTDeclaration(Type type) :
    type_specifier(type)
{}

CProgASTDeclaration::~CProgASTDeclaration()
{
    for(const CProgASTDeclarator* declarator : declarators)
    {
        delete declarator;
    }
}

// ----------------------------------------------------- Public Member Functions

void CProgASTDeclaration::add_declarator(CProgASTDeclarator* declarator)
{
    declarator->set_type(type_specifier);
    declarators.push_back(declarator);
}

std::string CProgASTDeclaration::build_ir(CFG* cfg) const
{
    for(const CProgASTDeclarator* declarator : declarators)
    {
        declarator->build_ir(cfg);
    }
    return ""; // ??
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTDeclarator                                                   //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTDeclarator::CProgASTDeclarator(CProgASTIdentifier* id, CProgASTAssignment* init) :
    identifier(id), initializer(init)
{}

CProgASTDeclarator::~CProgASTDeclarator()
{
    if(identifier)
        delete identifier;
    if(initializer)
        delete initializer;
}

// ----------------------------------------------------- Public Member Functions
void CProgASTDeclarator::set_type(Type type)
{
    type_specifier = type;
}

std::string CProgASTDeclarator::build_ir(CFG* cfg) const
{
    std::string name = identifier->getText();
    if(!cfg->is_declared(name))
    {
        cfg->add_to_symbol_table(name, type_specifier);
    }
    if(initializer != nullptr)
    {
        initializer->build_ir(cfg);
    }
    return ""; // ??
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTAssignment                                                   //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTAssignment::CProgASTAssignment(
        CProgASTIdentifier* identifier,
        CProgASTExpression* expression) :
    lhs_identifier(identifier),
    rhs_expression(expression)
{}

CProgASTAssignment::~CProgASTAssignment()
{
    delete lhs_identifier;
    delete rhs_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTAssignment::build_ir(CFG* cfg) const
{
    std::string name = lhs_identifier->getText();
    std::string init = rhs_expression->build_ir(cfg);
    //if(!cfg->tos.declared(name))
    {
        // error
    }
    cfg->current_bb->add_IRInstr(IRInstr::wmem, cfg->get_var_type(name), {name, init});
    return name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTPrePP : public CProgASTExpression                            //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTPrePP::CProgASTPrePP(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTPrePP::~CProgASTPrePP()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTPrePP::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    cfg->current_bb->add_IRInstr(IRInstr::pre_pp, cfg->get_var_type(exp_name), {exp_name});
    return exp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTPreMM : public CProgASTExpression                            //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTPreMM::CProgASTPreMM(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTPreMM::~CProgASTPreMM()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTPreMM::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    cfg->current_bb->add_IRInstr(IRInstr::pre_mm, cfg->get_var_type(exp_name), {exp_name});
    return exp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTPostPP : public CProgASTExpression                            //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTPostPP::CProgASTPostPP(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTPostPP::~CProgASTPostPP()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTPostPP::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    Type result_type = cfg->get_var_type(exp_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::post_pp, result_type, {tmp_name, exp_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTPostMM : public CProgASTExpression                            //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTPostMM::CProgASTPostMM(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTPostMM::~CProgASTPostMM()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTPostMM::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    Type result_type = cfg->get_var_type(exp_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::post_mm, result_type, {tmp_name, exp_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTBAnd : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTBAnd::CProgASTBAnd(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTBAnd::~CProgASTBAnd()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTBAnd::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::band, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTBOr : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTBOr::CProgASTBOr(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTBOr::~CProgASTBOr()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTBOr::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::bor, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTBXor : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTBXor::CProgASTBXor(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTBXor::~CProgASTBXor()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTBXor::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::bxor, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTBNot : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTBNot::CProgASTBNot(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTBNot::~CProgASTBNot()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTBNot::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    Type result_type = cfg->get_var_type(exp_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::bnot, result_type, {tmp_name, exp_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTAnd : public CProgASTExpression                              //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTAnd::CProgASTAnd(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTAnd::~CProgASTAnd()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTAnd::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::land, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTOr : public CProgASTExpression                               //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTOr::CProgASTOr(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTOr::~CProgASTOr()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTOr::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::lor, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTNot : public CProgASTExpression                              //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTNot::CProgASTNot(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTNot::~CProgASTNot()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTNot::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    Type result_type = cfg->get_var_type(exp_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::lnot, result_type, {tmp_name, exp_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTLessThan : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTLessThan::CProgASTLessThan(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTLessThan::~CProgASTLessThan()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTLessThan::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    cfg->current_bb->add_IRInstr(IRInstr::cmp_lt, Type::INT_64, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTLessThanOrEqual : public CProgASTExpression                  //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTLessThanOrEqual::CProgASTLessThanOrEqual(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTLessThanOrEqual::~CProgASTLessThanOrEqual()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTLessThanOrEqual::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    cfg->current_bb->add_IRInstr(IRInstr::cmp_le, Type::INT_64, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTGreaterThan : public CProgASTExpression                      //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTGreaterThan::CProgASTGreaterThan(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTGreaterThan::~CProgASTGreaterThan()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTGreaterThan::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    cfg->current_bb->add_IRInstr(IRInstr::cmp_gt, Type::INT_64, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTGreaterThanOrEqual : public CProgASTExpression               //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTGreaterThanOrEqual::CProgASTGreaterThanOrEqual(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTGreaterThanOrEqual::~CProgASTGreaterThanOrEqual()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTGreaterThanOrEqual::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    cfg->current_bb->add_IRInstr(IRInstr::cmp_ge, Type::INT_64, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTEqual : public CProgASTExpression                            //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTEqual::CProgASTEqual(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTEqual::~CProgASTEqual()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTEqual::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    cfg->current_bb->add_IRInstr(IRInstr::cmp_eq, Type::INT_64, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTNotEqual : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTNotEqual::CProgASTNotEqual(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTNotEqual::~CProgASTNotEqual()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTNotEqual::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    cfg->current_bb->add_IRInstr(IRInstr::cmp_ne, Type::INT_64, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}


////////////////////////////////////////////////////////////////////////////////
// class CProgASTAddition : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTAddition::CProgASTAddition(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTAddition::~CProgASTAddition()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTAddition::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::add, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTSubtraction : public CProgASTExpression                      //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTSubtraction::CProgASTSubtraction(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTSubtraction::~CProgASTSubtraction()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTSubtraction::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::sub, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTMultiplication : public CProgASTExpression                   //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTMultiplication::CProgASTMultiplication(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTMultiplication::~CProgASTMultiplication()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTMultiplication::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::mul, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTDivision : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTDivision::CProgASTDivision(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTDivision::~CProgASTDivision()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTDivision::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::div, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTModulo : public CProgASTExpression                           //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTModulo::CProgASTModulo(CProgASTExpression* lhs, CProgASTExpression* rhs) :
    lhs_operand(lhs), rhs_operand(rhs)
{}

CProgASTModulo::~CProgASTModulo()
{
    delete lhs_operand;
    delete rhs_operand;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTModulo::build_ir(CFG* cfg) const
{
    std::string lhs_name = lhs_operand->build_ir(cfg);
    std::string rhs_name = rhs_operand->build_ir(cfg);
    Type result_type = cfg->get_max_type(lhs_name, rhs_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::mod, result_type, {tmp_name, lhs_name, rhs_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTUnaryMinus : public CProgASTExpression                       //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTUnaryMinus::CProgASTUnaryMinus(CProgASTExpression* expression) :
    inner_expression(expression)
{}

CProgASTUnaryMinus::~CProgASTUnaryMinus()
{
    delete inner_expression;
}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTUnaryMinus::build_ir(CFG* cfg) const
{
    std::string exp_name = inner_expression->build_ir(cfg);
    Type result_type = cfg->get_var_type(exp_name);
    std::string tmp_name = cfg->create_new_tempvar(result_type);
    cfg->current_bb->add_IRInstr(IRInstr::neg, result_type, {tmp_name, exp_name});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTFunccall : public CProgASTExpression                         //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTFunccall::CProgASTFunccall(CProgASTIdentifier* identifier) :
    func_name(identifier)
{}

CProgASTFunccall::~CProgASTFunccall()
{
    delete func_name;
    for(CProgASTExpression* arg : args)
    {
        delete arg;
    }
}

// ----------------------------------------------------- Public Member Functions
void CProgASTFunccall::add_arg(CProgASTExpression* arg)
{
    args.push_back(arg);
}

std::string CProgASTFunccall::build_ir(CFG* cfg) const
{
    Type result_type = cfg->get_var_type(func_name->getText());
    std::string tmp_name = "";
    if (result_type != Type::VOID)
    {
        tmp_name = cfg->create_new_tempvar(result_type);
    }
    std::vector<std::string> params{tmp_name, func_name->getText()};
    for(CProgASTExpression* arg : args)
    {
        params.push_back(arg->build_ir(cfg));
    }
    cfg->current_bb->add_IRInstr(IRInstr::call, result_type, params);
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTIntLiteral : public CProgASTExpression                       //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTIntLiteral::CProgASTIntLiteral(int64_t val)
    : value(val)
{}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTIntLiteral::build_ir(CFG* cfg) const
{
    std::string tmp_name = cfg->create_new_tempvar(Type::INT_64);
    std::string literal_str = std::to_string(value);
    cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::INT_64, {tmp_name, literal_str});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTCharLiteral : public CProgASTExpression                      //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTCharLiteral::CProgASTCharLiteral(const std::string &val)
    : value(val)
{}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTCharLiteral::build_ir(CFG* cfg) const
{
    std::string tmp_name = cfg->create_new_tempvar(Type::CHAR);
    std::string literal_str;
    if (value.at(0) != '\\')
        literal_str = std::to_string(value.at(0));
    else
    {
        switch (value.at(1))
        {
            case 'a':
                literal_str = std::to_string(static_cast<unsigned int>('\a'));
            break;
            case 'b':
                literal_str = std::to_string(static_cast<unsigned int>('\b'));
            break;
            case 'f':
                literal_str = std::to_string(static_cast<unsigned int>('\f'));
            break;
            case 'n':
                literal_str = std::to_string(static_cast<unsigned int>('\n'));
            break;
            case 'r':
                literal_str = std::to_string(static_cast<unsigned int>('\r'));
            break;
            case 't':
                literal_str = std::to_string(static_cast<unsigned int>('\t'));
            break;
            case 'v':
                literal_str = std::to_string(static_cast<unsigned int>('\v'));
            break;
            case '\\':
                literal_str = std::to_string(static_cast<unsigned int>('\\'));
            break;
            case '\'':
                literal_str = std::to_string(static_cast<unsigned int>('\''));
            break;
            case '"':
                literal_str = std::to_string(static_cast<unsigned int>('\"'));
            break;
            case '?':
                literal_str = std::to_string(static_cast<unsigned int>('\?'));
            break;
        }
    }
    cfg->current_bb->add_IRInstr(IRInstr::ldconst, Type::CHAR, {tmp_name, literal_str});
    return tmp_name;
}

////////////////////////////////////////////////////////////////////////////////
// class CProgASTIdentifier                                                   //
////////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------- Constructor / Destructor
CProgASTIdentifier::CProgASTIdentifier(std::string identifier) :
    name(identifier)
{}

// ----------------------------------------------------- Public Member Functions
std::string CProgASTIdentifier::getText() const
{
    return name;
}

std::string CProgASTIdentifier::build_ir(CFG*) const
{
    /*if(!cfg->tos.declared(name))
    {
        // error
    }
    else
    {
        cfg->tos[name].used = true;
    }*/
    return name;
}
