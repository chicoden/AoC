with open("input.txt", "r") as file:
    input_string = file.read()

total = 0
for id_range in input_string.rstrip().split(","):
    start, end = map(int, id_range.split("-"))
    min_multiple = 1
    while True:
        next_min_multiple = min_multiple * 10
        multiplier = next_min_multiple + 1

        if min_multiple * multiplier > end:
            # no further ranges intersect the search range
            break

        # invalid IDs are in [min_multiple, max_multiple] * multiplier
        max_multiple = next_min_multiple - 1
        min_multiple_in_range = (start + multiplier - 1) // multiplier # ceil(start / multiplier)
        max_multiple_in_range = end // multiplier                      # floor(end / multiplier)
        start_multiple = max(min_multiple, min_multiple_in_range)
        end_multiple   = min(max_multiple, max_multiple_in_range)
        # [start_multiple, end_multiple] is the intersection between the range of invalid IDs and the range of multiples in [start, end]
        # we need to sum this range of multiples of multiplier

        if end_multiple >= start_multiple:
            # nonempty intersection
            # calculate sum of multiples and add to the total
            total += (end_multiple * (end_multiple + 1) - (start_multiple - 1) * start_multiple) // 2 * multiplier

        min_multiple = next_min_multiple

print("Sum of invalid IDs:", total)
