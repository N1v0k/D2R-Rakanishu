using System;

namespace Ryujinx.Common.Collections
{




    public class IntrusiveRedBlackTree<T> : IntrusiveRedBlackTreeImpl<T> where T : IntrusiveRedBlackTreeNode<T>, IComparable<T>
    {
        #region Public Methods







        public void Add(T node, T parent = null)
        {
            ArgumentNullException.ThrowIfNull(node);

            Insert(node, parent);
        }






        public void Remove(T node)
        {
            ArgumentNullException.ThrowIfNull(node);

            if (Delete(node) != null)
            {
                Count--;
            }
        }







        public T GetNode(T searchNode)
        {
            ArgumentNullException.ThrowIfNull(searchNode);

            T node = Root;
            while (node != null)
            {
                int cmp = searchNode.CompareTo(node);
                if (cmp < 0)
                {
                    node = node.Left;
                }
                else if (cmp > 0)
                {
                    node = node.Right;
                }
                else
                {
                    return node;
                }
            }

            return null;
        }

        #endregion

        #region Private Methods (BST)






        private void Insert(T node, T parent = null)
        {
            T newNode = parent != null ? InsertWithParent(node, parent) : BSTInsert(node);
            
            RestoreBalanceAfterInsertion(newNode);
        }










        private T BSTInsert(T newNode)
        {
            T parent = null;
            T node = Root;

            while (node != null)
            {
                parent = node;
                int cmp = newNode.CompareTo(node);
                if (cmp < 0)
                {
                    node = node.Left;
                }
                else if (cmp > 0)
                {
                    node = node.Right;
                }
                else
                {
                    return node;
                }
            }

            newNode.Parent = parent;
            if (parent == null)
            {
                Root = newNode;
            }
            else if (newNode.CompareTo(parent) < 0)
            {
                parent.Left = newNode;
                
                newNode.Successor = parent;
                
                if (parent.Predecessor != null)
                {
                    newNode.Predecessor = parent.Predecessor;
                    newNode.Predecessor.Successor = newNode;
                }
                
                parent.Predecessor = newNode;
            }
            else
            {
                parent.Right = newNode;
                
                newNode.Predecessor = parent;
                
                if (parent.Successor != null)
                {
                    newNode.Successor = parent.Successor;
                    newNode.Successor.Predecessor = newNode;
                }
                
                parent.Successor = newNode;
            }
            Count++;
            return newNode;
        }
        










        private T InsertWithParent(T newNode, T parent)
        {
            newNode.Parent = parent;

            if (newNode.CompareTo(parent) < 0)
            {
                parent.Left = newNode;
                
                newNode.Successor = parent;
                
                if (parent.Predecessor != null)
                {
                    newNode.Predecessor = parent.Predecessor;
                    parent.Predecessor = newNode;
                    newNode.Predecessor.Successor = newNode;
                }
                
                parent.Predecessor = newNode;
            }
            else
            {
                parent.Right = newNode;
                
                newNode.Predecessor = parent;
                
                if (parent.Successor != null)
                {
                    newNode.Successor = parent.Successor;
                    newNode.Successor.Predecessor = newNode;
                }
                
                parent.Successor = newNode;
            }

            Count++;
            return newNode;
        }






        private T Delete(T nodeToDelete)
        {
            if (nodeToDelete == null)
            {
                return null;
            }

            T old = nodeToDelete;
            T child;
            T parent;
            bool color;

            if (LeftOf(nodeToDelete) == null)
            {
                child = RightOf(nodeToDelete);
            }
            else if (RightOf(nodeToDelete) == null)
            {
                child = LeftOf(nodeToDelete);
            }
            else
            {
                T element = nodeToDelete.Successor;

                child = RightOf(element);
                parent = ParentOf(element);
                color = ColorOf(element);

                child?.Parent = parent;

                if (parent == null)
                {
                    Root = child;
                }
                else if (element == LeftOf(parent))
                {
                    parent.Left = child;
                }
                else
                {
                    parent.Right = child;
                }

                element.Color = old.Color;
                element.Left = old.Left;
                element.Right = old.Right;
                element.Parent = old.Parent;
                element.Predecessor = old.Predecessor;
                element.Predecessor?.Successor = element;

                if (ParentOf(old) == null)
                {
                    Root = element;
                }
                else if (old == LeftOf(ParentOf(old)))
                {
                    ParentOf(old).Left = element;
                }
                else
                {
                    ParentOf(old).Right = element;
                }

                LeftOf(old).Parent = element;

                if (RightOf(old) != null)
                {
                    RightOf(old).Parent = element;
                }

                if (child != null && color == Black)
                {
                    RestoreBalanceAfterRemoval(child);
                }

                return old;
            }

            parent = ParentOf(nodeToDelete);
            color = ColorOf(nodeToDelete);

            child?.Parent = parent;

            if (parent == null)
            {
                Root = child;
            }
            else if (nodeToDelete == LeftOf(parent))
            {
                parent.Left = child;
            }
            else
            {
                parent.Right = child;
            }

            if (child != null && color == Black)
            {
                RestoreBalanceAfterRemoval(child);
            }

            old.Successor?.Predecessor = old.Predecessor;
            old.Predecessor?.Successor = old.Successor;

            return old;
        }

        #endregion
    }

    public static class IntrusiveRedBlackTreeExtensions
    {






        public static TNode GetNodeByKey<TNode, TKey>(this IntrusiveRedBlackTree<TNode> tree, TKey key)
            where TNode : IntrusiveRedBlackTreeNode<TNode>, IComparable<TNode>, IComparable<TKey>
            where TKey : struct
        {
            TNode node = tree.RootNode;
            while (node != null)
            {
                int cmp = node.CompareTo(key);
                if (cmp < 0)
                {
                    node = node.Right;
                }
                else if (cmp > 0)
                {
                    node = node.Left;
                }
                else
                {
                    return node;
                }
            }

            return null;
        }
    }
}
