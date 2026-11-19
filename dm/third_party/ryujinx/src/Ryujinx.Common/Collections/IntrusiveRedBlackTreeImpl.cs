using System;

namespace Ryujinx.Common.Collections
{




    public class IntrusiveRedBlackTreeImpl<T> where T : IntrusiveRedBlackTreeNode<T>
    {
        protected const bool Black = true;
        protected const bool Red = false;
        protected T Root;

        internal T RootNode => Root;




        public int Count { get; protected set; }




        public void Clear()
        {
            Root = null;
            Count = 0;
        }






        internal static T SuccessorOf(T node)
        {
            if (node.Right != null)
            {
                return Minimum(node.Right);
            }

            T parent = node.Parent;
            while (parent != null && node == parent.Right)
            {
                node = parent;
                parent = parent.Parent;
            }

            return parent;
        }






        internal static T PredecessorOf(T node)
        {
            if (node.Left != null)
            {
                return Maximum(node.Left);
            }

            T parent = node.Parent;
            while (parent != null && node == parent.Left)
            {
                node = parent;
                parent = parent.Parent;
            }

            return parent;
        }






        protected static T Maximum(T node)
        {
            T tmp = node;
            while (tmp.Right != null)
            {
                tmp = tmp.Right;
            }

            return tmp;
        }







        protected static T Minimum(T node)
        {
            ArgumentNullException.ThrowIfNull(node);

            T tmp = node;
            while (tmp.Left != null)
            {
                tmp = tmp.Left;
            }

            return tmp;
        }

        protected void RestoreBalanceAfterRemoval(T balanceNode)
        {
            T ptr = balanceNode;

            while (ptr != Root && ColorOf(ptr) == Black)
            {
                if (ptr == LeftOf(ParentOf(ptr)))
                {
                    T sibling = RightOf(ParentOf(ptr));

                    if (ColorOf(sibling) == Red)
                    {
                        SetColor(sibling, Black);
                        SetColor(ParentOf(ptr), Red);
                        RotateLeft(ParentOf(ptr));
                        sibling = RightOf(ParentOf(ptr));
                    }

                    if (ColorOf(LeftOf(sibling)) == Black && ColorOf(RightOf(sibling)) == Black)
                    {
                        SetColor(sibling, Red);
                        ptr = ParentOf(ptr);
                    }
                    else
                    {
                        if (ColorOf(RightOf(sibling)) == Black)
                        {
                            SetColor(LeftOf(sibling), Black);
                            SetColor(sibling, Red);
                            RotateRight(sibling);
                            sibling = RightOf(ParentOf(ptr));
                        }

                        SetColor(sibling, ColorOf(ParentOf(ptr)));
                        SetColor(ParentOf(ptr), Black);
                        SetColor(RightOf(sibling), Black);
                        RotateLeft(ParentOf(ptr));
                        ptr = Root;
                    }
                }
                else
                {
                    T sibling = LeftOf(ParentOf(ptr));

                    if (ColorOf(sibling) == Red)
                    {
                        SetColor(sibling, Black);
                        SetColor(ParentOf(ptr), Red);
                        RotateRight(ParentOf(ptr));
                        sibling = LeftOf(ParentOf(ptr));
                    }

                    if (ColorOf(RightOf(sibling)) == Black && ColorOf(LeftOf(sibling)) == Black)
                    {
                        SetColor(sibling, Red);
                        ptr = ParentOf(ptr);
                    }
                    else
                    {
                        if (ColorOf(LeftOf(sibling)) == Black)
                        {
                            SetColor(RightOf(sibling), Black);
                            SetColor(sibling, Red);
                            RotateLeft(sibling);
                            sibling = LeftOf(ParentOf(ptr));
                        }

                        SetColor(sibling, ColorOf(ParentOf(ptr)));
                        SetColor(ParentOf(ptr), Black);
                        SetColor(LeftOf(sibling), Black);
                        RotateRight(ParentOf(ptr));
                        ptr = Root;
                    }
                }
            }

            SetColor(ptr, Black);
        }

        protected void RestoreBalanceAfterInsertion(T balanceNode)
        {
            SetColor(balanceNode, Red);
            while (balanceNode != null && balanceNode != Root && ColorOf(ParentOf(balanceNode)) == Red)
            {
                if (ParentOf(balanceNode) == LeftOf(ParentOf(ParentOf(balanceNode))))
                {
                    T sibling = RightOf(ParentOf(ParentOf(balanceNode)));

                    if (ColorOf(sibling) == Red)
                    {
                        SetColor(ParentOf(balanceNode), Black);
                        SetColor(sibling, Black);
                        SetColor(ParentOf(ParentOf(balanceNode)), Red);
                        balanceNode = ParentOf(ParentOf(balanceNode));
                    }
                    else
                    {
                        if (balanceNode == RightOf(ParentOf(balanceNode)))
                        {
                            balanceNode = ParentOf(balanceNode);
                            RotateLeft(balanceNode);
                        }

                        SetColor(ParentOf(balanceNode), Black);
                        SetColor(ParentOf(ParentOf(balanceNode)), Red);
                        RotateRight(ParentOf(ParentOf(balanceNode)));
                    }
                }
                else
                {
                    T sibling = LeftOf(ParentOf(ParentOf(balanceNode)));

                    if (ColorOf(sibling) == Red)
                    {
                        SetColor(ParentOf(balanceNode), Black);
                        SetColor(sibling, Black);
                        SetColor(ParentOf(ParentOf(balanceNode)), Red);
                        balanceNode = ParentOf(ParentOf(balanceNode));
                    }
                    else
                    {
                        if (balanceNode == LeftOf(ParentOf(balanceNode)))
                        {
                            balanceNode = ParentOf(balanceNode);
                            RotateRight(balanceNode);
                        }

                        SetColor(ParentOf(balanceNode), Black);
                        SetColor(ParentOf(ParentOf(balanceNode)), Red);
                        RotateLeft(ParentOf(ParentOf(balanceNode)));
                    }
                }
            }

            SetColor(Root, Black);
        }

        protected virtual void RotateLeft(T node)
        {
            if (node != null)
            {
                T right = RightOf(node);
                node.Right = LeftOf(right);
                node.Right?.Parent = node;

                T nodeParent = ParentOf(node);
                right.Parent = nodeParent;
                if (nodeParent == null)
                {
                    Root = right;
                }
                else if (node == LeftOf(nodeParent))
                {
                    nodeParent.Left = right;
                }
                else
                {
                    nodeParent.Right = right;
                }

                right.Left = node;
                node.Parent = right;
            }
        }

        protected virtual void RotateRight(T node)
        {
            if (node != null)
            {
                T left = LeftOf(node);
                node.Left = RightOf(left);
                node.Left?.Parent = node;

                T nodeParent = ParentOf(node);
                left.Parent = nodeParent;
                if (nodeParent == null)
                {
                    Root = left;
                }
                else if (node == RightOf(nodeParent))
                {
                    nodeParent.Right = left;
                }
                else
                {
                    nodeParent.Left = left;
                }

                left.Right = node;
                node.Parent = left;
            }
        }

        #region Safety-Methods








        protected static bool ColorOf(T node)
        {
            return node == null || node.Color;
        }








        protected static void SetColor(T node, bool color)
        {
            node?.Color = color;
        }






        protected static T LeftOf(T node)
        {
            return node?.Left;
        }






        protected static T RightOf(T node)
        {
            return node?.Right;
        }






        protected static T ParentOf(T node)
        {
            return node?.Parent;
        }

        #endregion
    }
}
