from RegionTypes.RegionType import RegionType
from testZ3 import run_tests
from RegionHandler import RegionHandler

if __name__ == '__main__':
    # run_tests()
    regions = [RegionType(), RegionType(), RegionType()]
    handler = RegionHandler(regions)
    handler.test_cells([0, 11, 11, 11, 1, 11, 11, 11])
