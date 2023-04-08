from py_code.RegionTypes.RegionTypeMaker import create
from py_code.RegionHandler import RegionHandler

if __name__ == '__main__':
    # run_tests()
    regions = [create("1"), create("1"), create("2")]
    handler = RegionHandler(regions)
    # result = handler.test_cells([0, 11, 11, 11, 1, 11, 11, 11])
    result = handler.recursive_test(4)
    print("Done")
