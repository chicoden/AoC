import list;

namespace io {
    namespace detail {
        void *fopen(NativeString path, NativeString flags) {
            __intrinsic_asm "(void*)fopen($0, $1)";
        }

        void fclose(void *stream) {
            __intrinsic_asm "fclose((FILE*)$0)";
        }

        int fgetc(void *stream) {
            __intrinsic_asm "fgetc((FILE*)$0)";
        }
    }

    struct File {
        void *handle;

        static Optional<File> open(NativeString path, NativeString flags) {
            let handle = detail::fopen(path, flags);
            if (!handle) return none;
            return File(handle);
        }

        void close() {
            detail::fclose(this.handle);
        }

        int getchar() {
            return detail::fgetc(this.handle);
        }

        size_t getline(inout list::List<uint8_t> buffer) {
            size_t initial_length = buffer.length;
            while (true) {
                int ch = this.getchar();
                if (ch == '\n' || ch < 0) { // newline or EOF
                    return buffer.length - initial_length; // return line length
                }

                buffer.append(reinterpret<uint8_t>(ch));
            }
        }
    }
}
