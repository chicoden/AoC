import io;
import list;

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

        ///
    }

    return 0;
}