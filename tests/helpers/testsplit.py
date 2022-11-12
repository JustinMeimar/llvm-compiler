import os
import sys

"""
To place #split token to separate .in .ins and .out (in order) files in test cases, 
"#split\n" token should be placed right after the last second ends
There should be two split token in the file
"""

def getAllTestsInDirectory(prefix):
    # return a pair (path, filename) for each test found
    results = []
    for file in os.listdir(prefix):
        full_path = prefix + file
        if os.path.isdir(full_path):
            dir_results = getAllTestsInDirectory(full_path + "/")
            for dir_result in dir_results:
                results.append(dir_result)
        else:
            results.append((full_path, file))
    return results


def main():
    # for each test case in test-source, create .in .out and .inStr files
    test_prefix = "./test-source/"
    out_prefix = "./../"
    out_middle = "nagcpp/"

    tests = getAllTestsInDirectory(test_prefix)
    for test_tuple in tests:
        test_path, test = test_tuple
        if test[-5:] == ".test":
            test_filename = test[:-5]
            test_file = open(test_path, "r")
            results = test_file.read().split("#split_token\n")
            for result in results:
                if result.endswith("#split_token"):
                    raise RuntimeError("ERROR: a section of input ends with '#split_token' instead of '#split_token\n', did you forget to put \n at the end?")
            test_file.close()
            
            with open(out_prefix + "input/" + out_middle + test_filename + ".in", "w+") as out:
                out.write(results[0])
            with open(out_prefix + "inStream/" + out_middle + test_filename + ".ins", "w+") as out:
                if (len(results) > 1):
                    out.write(results[1])
            with open(out_prefix + "output/" + out_middle + test_filename + ".out", "w+") as out:
                if (len(results) > 2):
                    out.write(results[2])
    

if __name__ == "__main__":
    main()