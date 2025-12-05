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

        size_t accessible_roll_count = 0;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (grid[y * cols + x] != ROLL) continue;

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
                    accessible_roll_count++;
                }
            }
        }

        printf("Number of rolls that can be accessed: %zu\n", accessible_roll_count);
    } else {
        printf("failed to open input file\n");
    }

    return 0;
}
