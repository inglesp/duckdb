#include "core_functions/scalar/list_functions.hpp"

#include "duckdb/function/lambda_functions.hpp"
#include "duckdb/planner/expression/bound_cast_expression.hpp"

namespace duckdb {

static unique_ptr<FunctionData> ListAnyBind(ClientContext &context, ScalarFunction &bound_function,
                                            vector<unique_ptr<Expression>> &arguments) {
	D_ASSERT(arguments.size() == 2);
	if (arguments[1]->GetExpressionClass() != ExpressionClass::BOUND_LAMBDA) {
		throw BinderException("Invalid lambda expression!");
	}

	auto &bound_lambda_expr = arguments[1]->Cast<BoundLambdaExpression>();

	if (bound_lambda_expr.lambda_expr->return_type != LogicalType::BOOLEAN) {
		auto cast_lambda_expr =
		    BoundCastExpression::AddCastToType(context, std::move(bound_lambda_expr.lambda_expr), LogicalType::BOOLEAN);
		bound_lambda_expr.lambda_expr = std::move(cast_lambda_expr);
	}

	arguments[0] = BoundCastExpression::AddArrayCastToList(context, std::move(arguments[0]));

	auto has_index = bound_lambda_expr.parameter_count == 2;
	auto bind_data = LambdaFunctions::ListLambdaBind(context, bound_function, arguments, has_index);
	bound_function.SetReturnType(LogicalType::BOOLEAN);
	return bind_data;
}

static LogicalType ListAnyBindLambda(ClientContext &context, const vector<LogicalType> &function_child_types,
                                     const idx_t parameter_idx) {
	return LambdaFunctions::BindBinaryChildren(function_child_types, parameter_idx);
}

ScalarFunction ListAnyFun::GetFunction() {
	ScalarFunction fun({LogicalType::LIST(LogicalType::ANY), LogicalType::LAMBDA}, LogicalType::BOOLEAN,
	                   LambdaFunctions::ListAnyFunction, ListAnyBind, nullptr, nullptr);

	fun.SetNullHandling(FunctionNullHandling::SPECIAL_HANDLING);
	fun.SetSerializeCallback(ListLambdaBindData::Serialize);
	fun.SetDeserializeCallback(ListLambdaBindData::Deserialize);
	fun.SetBindLambdaCallback(ListAnyBindLambda);

	return fun;
}

} // namespace duckdb
