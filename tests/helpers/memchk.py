import subprocess
import os
import time
import sys

def run_program(args):
    for arg in args:
        print(arg, end = " ")
    print(flush = True)
    return subprocess.check_output(args)

def main():

    root_path = "../../"
    testin_prefix = root_path + "tests/input/"
    libgazrt_path = root_path + "bin/libgazrt.so"

    os.environ["LD_PRELOAD"] = libgazrt_path

    test_paths = []
    for tests_dir in os.listdir(testin_prefix):
        for test_dir in os.listdir(testin_prefix + tests_dir):
            test_paths.append(testin_prefix + tests_dir + "/" + test_dir)

    for test_path in test_paths:
        print("\n\n\ntesting file:" + test_path, file=sys.stderr, flush=True)

        try:
            # to .ll
            llFile = "../gazprea_program.ll"
            args = [root_path + "bin/gazc", test_path, llFile]
            run_program(args)

            # to .o
            oFile = "../gazprea_program.o"
            args = ["llc", "-filetype=obj", llFile, "-o", oFile]
            run_program(args)

            # to binary
            binaryFile = "../gazprea_program"
            args = ["clang", oFile, libgazrt_path, "-o", binaryFile]
            run_program(args)

            # run program
            args = ["valgrind", binaryFile]
            print(run_program(args).decode("UTF-8"), file=sys.stderr, flush=True)

            time.sleep(0.1)
        except subprocess.CalledProcessError as e:
            print(str(e), file=sys.stderr, flush=True)

if __name__ == "__main__":
    main()