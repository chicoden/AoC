with open("input.txt", "r") as file:
    input_string = file.read()

dial_position = 50
passcode = 0
for command in input_string.split():
    ticks = int(command[1:])
    if command[0] == "L":
        ticks = -ticks
    # alternative is an R which means a positive increment

    dial_position += ticks
    dial_position %= 100 # euclidean modulo to emulate wrapping behavior of dial

    if dial_position == 0:
        passcode += 1 # passcode is the number of times the dial ends a rotation on a 0

print("Passcode:", passcode)
