from py_code.RegionTypes.EqualsRegionType import EqualsRegionType


# This is in a different file to everything else because putting it in RegionType.py makes a circular import loop
def create(name: str):
    """
    Tries to initialize a RegionType from the given name
    Does very little work to make sure the RegionType is actually valid or that it won't throw an error.
    Also mostly works by just calling the initializers for other region types.
    """
    return EqualsRegionType(name.split("/"))
