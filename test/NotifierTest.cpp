#include <gtest/gtest.h>

#include "Notifier.h"

TEST(Notifier, NotifyLambdaInHandle) {
	Notifier<int> notifier;

	bool called = false;

	auto l = notifier.AddEventListener(1, [&called](EventType, int) {
		called = true;
	});
	notifier.NotifyListeners(1, 0);

	ASSERT_TRUE(called);
}
