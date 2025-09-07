import sys

FILE_NAME = sys.argv[1]

objVertex = []
objEdges = []
objFaces = []
objNormals = []
objNormalsFaceIdx = []
objTypeSplitFace = []
objFaceOffset = 0

def printOjbInfo():
    s = f"/* defines for object */\n"

    s += f"#define VERTEX_SIZE {len(objVertex):d}\n"
    s += f"#define EDGE_SIZE {len(objEdges):d}\n"
    s += f"#define FACE_SIZE {len(objFaces):d}\n"
    s += f"#define NORMAL_SIZE {len(objNormals):d}\n"
    s += f"#define OBJ_TYPES_N {len(objTypeSplitFace):d}\n"


    s += f"float objectVertex[VERTEX_SIZE*4] = {{\n"
    for v in objVertex:
        s += '\t' + ', '.join(map(lambda x: f"{x:f}", v)) + ',\n'
    s += '};\n'

    s += f"uint objectEdge[EDGE_SIZE*2] =     {{\n"
    for v_i, v in enumerate(objEdges):
        s += '\t' + ', '.join(map(lambda x: f"{x:d}", v)) + ','
        if ((v_i+1) % 4) == 0:
            s += '\n'
    s += '};\n'

    s += f"uint objectFace[FACE_SIZE*3] =     {{\n"
    for v_i, v in enumerate(objFaces):
        s += '\t' + ', '.join(map(lambda x: f"{x:d}", v)) + ','
        if ((v_i+1) % 2) == 0:
            s += '\n'
    s += '};\n'

    s += f"float objectNormal[NORMAL_SIZE*4] =     {{\n"
    for v in objNormals:
        s += '\t' + ', '.join(map(lambda x: f"{x:f}", v)) + ',\n'
    s += '};\n'

    s += f"uint objectNormalFaceIdx[FACE_SIZE] =     {{\n"
    for v in objNormalsFaceIdx:
        s += f"{v:d}, "
    s += '};\n'

    s += f"uint objectTypeSplitFace[OBJ_TYPES_N] =     {{\n"
    for v in objTypeSplitFace:
        s += f"{v:d}, "
    s += '};\n'

    print(s)

def filterEdgeByFaceNormal():
    """
        Removes edges which belongs to two faces with the same normal vector
        That would mean it's not really an "edge" per say
    """
    newObjE = []
    for edge in objEdges:
        lastFaceIdx = None
        normalEqual = False         # if two faces with this edge has the same normal
        for faceIdx, face in enumerate(objFaces):
            # print(edge, face)
            if face.count(edge[0]) + face.count(edge[1]) >= 2:
                if lastFaceIdx is None:
                    lastFaceIdx = faceIdx
                else:
                    normA = objNormals[objNormalsFaceIdx[lastFaceIdx]]
                    normB = objNormals[objNormalsFaceIdx[faceIdx]]
                    lastFaceIdx = None
                    # print(normA, normB, normA == normB)
                    if normA == normB:
                        normalEqual = True
                        break
    
        if normalEqual is False:
            newObjE.append(edge)
    
    return newObjE

with open(FILE_NAME, 'r') as f:
    for line in f.readlines():
        line = line.strip()
        line = line.split(' ')

        token = line[0]

        if token == '#':
            continue
        elif token == 'o':
            if len(objFaces) != 0:
                objTypeSplitFace.append(len(objFaces))
        elif token == 'v':
            vertex = [float(i) for i in line[1:]]
            if len(vertex) == 3:
                vertex.append(1.0)
            objVertex.append(vertex)
        elif token == 'vn':
            vertex = [float(i) for i in line[1:]]
            if len(vertex) == 3:
                vertex.append(1.0)
            objNormals.append(vertex)
        elif token == 'f':
            faces = [int(i.split('//')[0])-1 for i in line[1:]]
            faceNorm = [int(i.split('//')[1])-1 for i in line[1:]]

            if len(faces) == 3:
                objFaces.append(faces)
            else:
                raise UserWarning("All faces must be pre-triangulated")
                # objFaces.append([0x100, 0x100, 0x100, 0x100])
            
            # only add if all values on normal vector match (not sure why they wouldn't)
            if sum(faceNorm) == faceNorm[0]*len(faceNorm):
                objNormalsFaceIdx.append(faceNorm[0])

            edges = list(zip(faces, faces[1:] + faces[:1]))
            for e in edges:
                if e in objEdges:
                    continue
                if e[::-1] in objEdges:
                    continue
                objEdges.append(e)

# add last object to the object filter
objTypeSplitFace.append(len(objFaces))
# print(filterEdgeByFaceNormal())
objEdges = filterEdgeByFaceNormal()
printOjbInfo()
