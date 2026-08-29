CC := clang
CFLAGS := -std=c99 -Wall -Wextra -Werror -Iinclude
BUILD := build

.PHONY: test clean

test: $(BUILD)/test_joystick_input
	$(BUILD)/test_joystick_input

$(BUILD)/test_joystick_input: tests/test_joystick_input.c src/joystick_input.c include/joystick_input.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) tests/test_joystick_input.c src/joystick_input.c -o $@

clean:
	rm -rf $(BUILD)
