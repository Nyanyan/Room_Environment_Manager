s = []

while True:
    ss = input()
    if ss == '':
        break
    s.append(ss)


for line in s:
    print('"', end='')
    for c in line:
        if c == '\\':
            print('\\\\', end='')
        elif c == '"':
            print('\\"', end='')
        else:
            print(c, end='')
    print('",')