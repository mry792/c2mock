#include "c2mm/mock/Mock_Function.hpp"

#include <functional>
#include <optional>
#include <string>
#include <tuple>

#include <catch2/catch_test_macros.hpp>

#include "c2mm/matchers/Any.hpp"
#include "c2mm/matchers/Comparison_Matcher.hpp"
#include "c2mm/mock/reporters/Mock.hpp"

SCENARIO ("If all calls are consumed, Mock_Function doesn't fail.") {
    GIVEN ("a Mock_Function") {
        using c2mm::mock::Mock_Function;
        auto func_ptr = std::make_unique<Mock_Function<void(int, double)>>();
        auto& func = *func_ptr;

        WHEN ("some calls are logged") {
            func(7, 4.2);
            func(-4, 1.1);

            AND_WHEN ("those calls are all matched") {
                using c2mm::matchers::less_than;
                func.check_called(less_than(8), 4.2);
                func.require_called(-4, less_than(2));

                THEN ("Mock_Function::~Mock_Function() doesn't fail") {
                    func_ptr.reset();
                }
            }
        }
    }
}

namespace reporters = c2mm::mock::reporters;
SCENARIO ("Mock_Function fails when calls aren't matched.") {
    GIVEN ("a Mock_Function") {
        using c2mm::mock::Mock_Function;
        using Func = Mock_Function<void(double, int), reporters::Mock_Ref>;

        reporters::Mock mock_reporter{};

        auto func_ptr = std::make_unique<Func>(std::ref(mock_reporter));
        auto& func = *func_ptr;
        (void)func;

        WHEN ("some calls are logged") {
            func(2.2, 4);
            func(3.3, -2);

            AND_WHEN ("not all calls are matched") {
                using c2mm::matchers::greater_than;
                func.check_called(greater_than(2), greater_than(0));

                THEN ("Mock_Function::~Mock_Function() fails") {
                    func_ptr.reset();
                    mock_reporter.check_called(
                        "unconsumed call with args:\n"
                        "  0: 3.3\n"
                        "  1: -2\n"
                    );
                }
            }

            AND_WHEN ("unmatched .validate_call() fails") {
                using c2mm::matchers::less_than;
                func.validate_called(
                    std::ref(mock_reporter),
                    less_than(3),
                    less_than(-2)
                );

                THEN ("the failure is reported") {
                    mock_reporter.check_called(
                        "No call where arguments:\n"
                        "  0: is < 3\n"
                        "  1: is < -2\n"
                    );

                    func_ptr.reset();
                    mock_reporter.check_called(
                        "unconsumed call with args:\n"
                        "  0: 2.2\n"
                        "  1: 4\n"
                    );
                    mock_reporter.check_called(
                        "unconsumed call with args:\n"
                        "  0: 3.3\n"
                        "  1: -2\n"
                    );
                }
            }
        }
    }
}

SCENARIO ("Mock_Function - Unmatched calls invoke the default action.") {
    using c2mm::matchers::_;

    GIVEN ("a Mock_Function") {
        using c2mm::mock::Mock_Function;
        using Func = Mock_Function<void(double, int), reporters::Mock_Ref>;

        reporters::Mock call_log_reporter{};
        Mock_Function<void(double, int)> mock_default_action{};

        Func test_func{call_log_reporter};
        test_func.default_action(std::ref(mock_default_action));

        WHEN ("the mock function is invoked") {
            test_func(-1.3, -2);
            test_func(86.9, 18);
            // TODO: "allow call"
            call_log_reporter.on_call(_);
            call_log_reporter.on_call(_);

            THEN ("the default action was invoked") {
                mock_default_action.check_called(86.9, 18);
                mock_default_action.check_called(-1.3, -2);
            }
        }

        AND_GIVEN ("an expectation") {
            test_func.on_call(1.1, 2);

            WHEN ("the mock function is invoked") {
                test_func(3.3, 4);
                test_func(1.1, 2);
                call_log_reporter.on_call(_);

                THEN ("the default action was invoked only for the unmatched calls") {
                    mock_default_action.check_called(3.3, 4);
                }
            }
        }
    }
}
