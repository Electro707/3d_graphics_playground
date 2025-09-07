import numpy as np

FILE_NAME = 'camera1.obj'

objVertex = []
objEdges = []
objFaceOffset = 0

def toQ15(v: float):
    if v > 1.0:
        raise UserWarning()
    v = v * (2**15) * 0.95
    v = np.round(v)
    if v > 0x7FFF:
        v = 0x7FFF
    return int(v)

def printOjbInfo():
    s = f"/* defines for object */\n"
    s += f"q15 objectVertex[{len(objVertex):d}*4] = {{\n"
    for v in objVertex:
        s += '\t' + ', '.join(map(lambda x: f"{toQ15(x):d}", v)) + ',\n'
    s += '};\n'

    s += f"uint8_t objectEdge[{len(objEdges):d}*2] =     {{\n"
    for v_i, v in enumerate(objEdges):
        s += '\t' + ', '.join(map(lambda x: f"{x-1:d}", v)) + ','
        if ((v_i+1) % 4) == 0:
            s += '\n'
    s += '};\n'

    print(s)

with open(FILE_NAME, 'r') as f:
    for line in f.readlines():
        line = line.strip()
        line = line.split(' ')

        token = line[0]

        if token == '#':
            continue
        elif token == 'o':
            continue
        elif token == 'v':
            vertex = [float(i) for i in line[1:]]
            if len(vertex) == 3:
                vertex.append(1.0)
            objVertex.append(vertex)
        elif token == 'f':
            faces = [int(i.split('//')[0]) for i in line[1:]]
            edges = list(zip(faces, faces[1:] + faces[:1]))
            for e in edges:
                if e in objEdges:
                    continue
                if e[::-1] in objEdges:
                    continue
                objEdges.append(e)

objVertex = np.array(objVertex)
objVertex[:, :-1] /= np.max(objVertex[:, :-1])

printOjbInfo()
