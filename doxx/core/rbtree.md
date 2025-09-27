Red-Black Tree (RBT)
    A RBT is a self-balancing binary search tree.
    Each node is assigned with a color (black or red). 
    A set of rules specifies how these colors must be arranged.

Sentinel Nodes in RBT
    A sentinel node is a leaf that does not contain a value.
    Sentinel become relevant for the algorithms later on, e.g,
    to determine colors of uncle nodes.

Red-Black Tree Properties
The following rules enforce the red-black tree balance:
    1) Each node is either red or black.
    2) The root is black
    3) All sentinel leaves are black.
    4) A red node must not have red children.
    5) All paths from a node to the leaves below contain the same number of black nodes.
