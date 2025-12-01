# update to use method 0x434C49434B
# passcode includes count of times dial passes over 0 during rotations

with open("input.txt", "r") as file:
    input_string = file.read()

dial_position = 50
passcode = 0
for command in input_string.split():
    ticks = int(command[1:])
    flipped = command[0] == "L"

    if flipped:
        # flip so that a left rotation becomes a right rotation
        dial_position = -dial_position % 100

    passes_over_zero, dial_position = divmod(dial_position + ticks, 100)
    passcode += passes_over_zero

    if flipped:
        # flip again to restore original orientation
        dial_position = -dial_position % 100

print("Passcode:", passcode)
