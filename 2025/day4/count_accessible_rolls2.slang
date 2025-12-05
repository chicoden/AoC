import io;
import list;

static const uint8_t ROLL = '@';
static const int ACCESSIBILITY_LIMIT = 3;

export __extern_cpp int main(int argc, NativeString *argv) {
    if (let file = io::File::open("input.txt", "rb")) {
        defer file.close();

        var grid = list::List<uint8_t>(0);
        defer grid.dispose();

        size_t cols = 0;
        size_t rows = 0;
        while (true) {
            size_t line_length = file.getline(grid);
            if (line_length == 0) break;
            cols = line_length;
            rows++;
        }

        size_t total_rolls_removed = 0;
        while (true) {
            size_t rolls_removed_this_iter = 0;
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    size_t cell_index = y * cols + x;
                    if (grid[cell_index] != ROLL) continue;

                    int neighboring_roll_count = 0;
                    for (int y_offset = -1; y_offset <= 1; y_offset++) {
                        for (int x_offset = -1; x_offset <= 1; x_offset++) {
                            if (x_offset == 0 && y_offset == 0) continue;

                            int neighbor_x = x + x_offset;
                            int neighbor_y = y + y_offset;
                            if (
                                neighbor_x < 0 || neighbor_x >= cols ||
                                neighbor_y < 0 || neighbor_y >= rows
                            ) continue;

                            if (grid[neighbor_y * cols + neighbor_x] == ROLL) {
                                neighboring_roll_count++;
                            }
                        }
                    }

                    if (neighboring_roll_count <= ACCESSIBILITY_LIMIT) {
                        grid[cell_index] = '.';
                        rolls_removed_this_iter++;
                    }
                }
            }

            if (rolls_removed_this_iter == 0) break;
            total_rolls_removed += rolls_removed_this_iter;
        }

        printf("Total rolls removed: %zu\n", total_rolls_removed);
    } else {
        printf("failed to open input file\n");
    }

    return 0;
}
