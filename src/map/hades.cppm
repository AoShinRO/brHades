// Este modulo vai ser usado para criar funçoes globais reutilizaveis
module;
export module hades;

import <algorithm>;
import <limits>;

// Cap input to numeric limits of target value and safecast it
// @param input: numeric value
// @return static_cast<decltype(T)> capped min/max value
export template <typename TargetType, typename InputType>
TargetType hades_cast(InputType input) {
	return static_cast<TargetType>(
		std::clamp(input,
			static_cast<InputType>(std::numeric_limits<TargetType>::min()),
			static_cast<InputType>(std::numeric_limits<TargetType>::max())));
}
