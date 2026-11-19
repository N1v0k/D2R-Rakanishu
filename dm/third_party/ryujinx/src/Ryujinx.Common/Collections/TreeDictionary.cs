using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;

namespace Ryujinx.Common.Collections
{





    public class TreeDictionary<TKey, TValue> : IntrusiveRedBlackTreeImpl<Node<TKey, TValue>>, IDictionary<TKey, TValue> where TKey : IComparable<TKey>
    {
        #region Public Methods







        public TValue Get(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            Node<TKey, TValue> node = GetNode(key);

            if (node == null)
            {
                return default;
            }

            return node.Value;
        }









        public void Add(TKey key, TValue value)
        {
            ArgumentNullException.ThrowIfNull(key);
            ArgumentNullException.ThrowIfNull(value);

            Insert(key, value);
        }






        public void Remove(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            if (Delete(key) != null)
            {
                Count--;
            }
        }







        public TKey Floor(TKey key)
        {
            Node<TKey, TValue> node = FloorNode(key);
            if (node != null)
            {
                return node.Key;
            }

            return default;
        }







        public TKey Ceiling(TKey key)
        {
            Node<TKey, TValue> node = CeilingNode(key);
            if (node != null)
            {
                return node.Key;
            }

            return default;
        }






        public TKey SuccessorOf(TKey key)
        {
            Node<TKey, TValue> node = GetNode(key);
            if (node != null)
            {
                Node<TKey, TValue> successor = node.Successor;

                return successor != null ? successor.Key : default;
            }

            return default;
        }






        public TKey PredecessorOf(TKey key)
        {
            Node<TKey, TValue> node = GetNode(key);
            if (node != null)
            {
                Node<TKey, TValue> predecessor = node.Predecessor;

                return predecessor != null ? predecessor.Key : default;
            }

            return default;
        }






        public List<KeyValuePair<TKey, TValue>> AsLevelOrderList()
        {
            List<KeyValuePair<TKey, TValue>> list = [];

            Queue<Node<TKey, TValue>> nodes = new();

            if (this.Root != null)
            {
                nodes.Enqueue(this.Root);
            }

            while (nodes.TryDequeue(out Node<TKey, TValue> node))
            {
                list.Add(new KeyValuePair<TKey, TValue>(node.Key, node.Value));
                if (node.Left != null)
                {
                    nodes.Enqueue(node.Left);
                }

                if (node.Right != null)
                {
                    nodes.Enqueue(node.Right);
                }
            }

            return list;
        }





        public List<KeyValuePair<TKey, TValue>> AsList()
        {
            List<KeyValuePair<TKey, TValue>> list = [];

            AddToList(Root, list);

            return list;
        }

        #endregion

        #region Private Methods (BST)






        private static void AddToList(Node<TKey, TValue> node, List<KeyValuePair<TKey, TValue>> list)
        {
            if (node == null)
            {
                return;
            }

            AddToList(node.Left, list);

            list.Add(new KeyValuePair<TKey, TValue>(node.Key, node.Value));

            AddToList(node.Right, list);
        }







        private Node<TKey, TValue> GetNode(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            Node<TKey, TValue> node = Root;
            while (node != null)
            {
                int cmp = key.CompareTo(node.Key);
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








        private void Insert(TKey key, TValue value)
        {
            Node<TKey, TValue> newNode = BSTInsert(key, value);
            RestoreBalanceAfterInsertion(newNode);
        }











        private Node<TKey, TValue> BSTInsert(TKey key, TValue value)
        {
            Node<TKey, TValue> parent = null;
            Node<TKey, TValue> node = Root;

            while (node != null)
            {
                parent = node;
                int cmp = key.CompareTo(node.Key);
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
                    node.Value = value;
                    return node;
                }
            }

            Node<TKey, TValue> newNode = new(key, value, parent);
            if (parent == null)
            {
                Root = newNode;
            }
            else if (key.CompareTo(parent.Key) < 0)
            {
                parent.Left = newNode;
            }
            else
            {
                parent.Right = newNode;
            }

            Count++;
            return newNode;
        }






        private Node<TKey, TValue> Delete(TKey key)
        {

            Node<TKey, TValue> nodeToDelete = GetNode(key);

            if (nodeToDelete == null)
            {
                return null;
            }

            Node<TKey, TValue> replacementNode;

            if (LeftOf(nodeToDelete) == null || RightOf(nodeToDelete) == null)
            {
                replacementNode = nodeToDelete;
            }
            else
            {
                replacementNode = PredecessorOf(nodeToDelete);
            }

            Node<TKey, TValue> tmp = LeftOf(replacementNode) ?? RightOf(replacementNode);

            tmp?.Parent = ParentOf(replacementNode);

            if (ParentOf(replacementNode) == null)
            {
                Root = tmp;
            }
            else if (replacementNode == LeftOf(ParentOf(replacementNode)))
            {
                ParentOf(replacementNode).Left = tmp;
            }
            else
            {
                ParentOf(replacementNode).Right = tmp;
            }

            if (replacementNode != nodeToDelete)
            {
                nodeToDelete.Key = replacementNode.Key;
                nodeToDelete.Value = replacementNode.Value;
            }

            if (tmp != null && ColorOf(replacementNode) == Black)
            {
                RestoreBalanceAfterRemoval(tmp);
            }

            return replacementNode;
        }







        private Node<TKey, TValue> FloorNode(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            Node<TKey, TValue> tmp = Root;

            while (tmp != null)
            {
                int cmp = key.CompareTo(tmp.Key);
                if (cmp > 0)
                {
                    if (tmp.Right != null)
                    {
                        tmp = tmp.Right;
                    }
                    else
                    {
                        return tmp;
                    }
                }
                else if (cmp < 0)
                {
                    if (tmp.Left != null)
                    {
                        tmp = tmp.Left;
                    }
                    else
                    {
                        Node<TKey, TValue> parent = tmp.Parent;
                        Node<TKey, TValue> ptr = tmp;
                        while (parent != null && ptr == parent.Left)
                        {
                            ptr = parent;
                            parent = parent.Parent;
                        }

                        return parent;
                    }
                }
                else
                {
                    return tmp;
                }
            }

            return null;
        }







        private Node<TKey, TValue> CeilingNode(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            Node<TKey, TValue> tmp = Root;

            while (tmp != null)
            {
                int cmp = key.CompareTo(tmp.Key);
                if (cmp < 0)
                {
                    if (tmp.Left != null)
                    {
                        tmp = tmp.Left;
                    }
                    else
                    {
                        return tmp;
                    }
                }
                else if (cmp > 0)
                {
                    if (tmp.Right != null)
                    {
                        tmp = tmp.Right;
                    }
                    else
                    {
                        Node<TKey, TValue> parent = tmp.Parent;
                        Node<TKey, TValue> ptr = tmp;
                        while (parent != null && ptr == parent.Right)
                        {
                            ptr = parent;
                            parent = parent.Parent;
                        }

                        return parent;
                    }
                }
                else
                {
                    return tmp;
                }
            }

            return null;
        }

        #endregion

        #region Interface Implementations


        public bool ContainsKey(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            return GetNode(key) != null;
        }

        bool IDictionary<TKey, TValue>.Remove(TKey key)
        {
            int count = Count;
            Remove(key);
            return count > Count;
        }

        public bool TryGetValue(TKey key, [MaybeNullWhen(false)] out TValue value)
        {
            ArgumentNullException.ThrowIfNull(key);

            Node<TKey, TValue> node = GetNode(key);
            value = node != null ? node.Value : default;
            return node != null;
        }

        public void Add(KeyValuePair<TKey, TValue> item)
        {
            ArgumentNullException.ThrowIfNull(item.Key);

            Add(item.Key, item.Value);
        }

        public bool Contains(KeyValuePair<TKey, TValue> item)
        {
            if (item.Key == null)
            {
                return false;
            }

            Node<TKey, TValue> node = GetNode(item.Key);
            if (node != null)
            {
                return node.Key.Equals(item.Key) && node.Value.Equals(item.Value);
            }

            return false;
        }

        public void CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            if (arrayIndex < 0 || array.Length - arrayIndex < this.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(arrayIndex));
            }

            SortedList<TKey, TValue> list = GetKeyValues();

            int offset = 0;

            for (int i = arrayIndex; i < array.Length && offset < list.Count; i++)
            {
                array[i] = new KeyValuePair<TKey, TValue>(list.Keys[i], list.Values[i]);
                offset++;
            }
        }

        public bool Remove(KeyValuePair<TKey, TValue> item)
        {
            Node<TKey, TValue> node = GetNode(item.Key);

            if (node == null)
            {
                return false;
            }

            if (node.Value.Equals(item.Value))
            {
                int count = Count;
                Remove(item.Key);
                return count > Count;
            }

            return false;
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return GetKeyValues().GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetKeyValues().GetEnumerator();
        }

        public ICollection<TKey> Keys => GetKeyValues().Keys;

        public ICollection<TValue> Values => GetKeyValues().Values;

        public bool IsReadOnly => false;

        public TValue this[TKey key]
        {
            get => Get(key);
            set => Add(key, value);
        }

        #endregion

        #region Private Interface Helper Methods





        private SortedList<TKey, TValue> GetKeyValues()
        {
            SortedList<TKey, TValue> set = new();
            Queue<Node<TKey, TValue>> queue = new();
            if (Root != null)
            {
                queue.Enqueue(Root);
            }

            while (queue.TryDequeue(out Node<TKey, TValue> node))
            {
                set.Add(node.Key, node.Value);
                if (null != node.Left)
                {
                    queue.Enqueue(node.Left);
                }

                if (null != node.Right)
                {
                    queue.Enqueue(node.Right);
                }
            }

            return set;
        }

        #endregion
    }






    public class Node<TKey, TValue> : IntrusiveRedBlackTreeNode<Node<TKey, TValue>> where TKey : IComparable<TKey>
    {
        internal TKey Key;
        internal TValue Value;

        internal Node(TKey key, TValue value, Node<TKey, TValue> parent)
        {
            Key = key;
            Value = value;
            Parent = parent;
        }
    }
}
