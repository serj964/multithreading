from random import randint

size = 4096
with open("first", "w") as first:
    with open("second", "w") as second:
        first.write('{size} {size}\n'.format(size=size))
        second.write('{size} {size}\n'.format(size=size))
        for i in range(size):
            for j in range(size):
                first.write("1 ")
                second.write("0 ")
                #first.write(str(randint(0, 10)) + " ")
                #second.write(str(randint(0, 10)) + " ")
            first.write("\n")
            second.write("\n")
