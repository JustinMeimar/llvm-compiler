import os
import sys

"""
remove all test cases splitted by testsplit.py
"""

def cleanFolder(folder_path, expected_suffix):
    for file in os.listdir(folder_path):
        if file.endswith(expected_suffix):
            os.remove(folder_path + file)


def main():
    out_prefix = "./../"
    out_middle = "nagcpp/"

    cleanFolder(out_prefix + "input/" + out_middle, ".in")
    cleanFolder(out_prefix + "inStream/" + out_middle, ".ins")
    cleanFolder(out_prefix + "output/" + out_middle, ".out")
    

if __name__ == "__main__":
    main()