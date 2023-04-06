from datetime import datetime

from RegionTypes.RegionTypeMaker import create
from testZ3 import run_tests
from RegionHandler import RegionHandler

if __name__ == '__main__':
    # run_tests()
    regions = [create("1"), create("1"), create("2")]
    handler = RegionHandler(regions)
    start = datetime.utcnow()
    print("Start time:", start)
    result = handler.test_cells([0, 11, 11, 11, 1, 11, 11, 11])
    end = datetime.utcnow()
    print("End time:", end)
    print("Time taken:", end - start)
