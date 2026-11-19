using System;
using System.Collections.Generic;
using System.Linq;

namespace Ryujinx.Common.Collections
{





    public class IntervalTree<TKey, TValue> : IntrusiveRedBlackTreeImpl<IntervalTreeNode<TKey, TValue>> where TKey : IComparable<TKey>
    {
        private const int ArrayGrowthSize = 32;

        #region Public Methods








        public int Get(TKey key, ref TValue[] overlaps)
        {
            if (!typeof(TKey).IsValueType)
            {
                ArgumentNullException.ThrowIfNull(key);
            }

            IntervalTreeNode<TKey, TValue> node = GetNode(key);

            if (node == null)
            {
                return 0;
            }

            if (node.Values.Count > overlaps.Length)
            {
                Array.Resize(ref overlaps, node.Values.Count);
            }

            int overlapsCount = 0;
            foreach (RangeNode<TKey, TValue> value in node.Values)
            {
                overlaps[overlapsCount++] = value.Value;
            }

            return overlapsCount;
        }










        public int Get(TKey start, TKey end, ref TValue[] overlaps, int overlapCount = 0)
        {
            ArgumentNullException.ThrowIfNull(start);
            ArgumentNullException.ThrowIfNull(end);

            GetValues(Root, start, end, ref overlaps, ref overlapCount);

            return overlapCount;
        }








        public void Add(TKey start, TKey end, TValue value)
        {
            ArgumentNullException.ThrowIfNull(start);
            ArgumentNullException.ThrowIfNull(end);
            ArgumentNullException.ThrowIfNull(value);

            Insert(start, end, value);
        }








        public int Remove(TKey key, TValue value)
        {
            if (!typeof(TKey).IsValueType)
            {
                ArgumentNullException.ThrowIfNull(key);
            }

            int removed = Delete(key, value);

            Count -= removed;

            return removed;
        }





        public List<RangeNode<TKey, TValue>> AsList()
        {
            List<RangeNode<TKey, TValue>> list = [];

            AddToList(Root, list);

            return list;
        }

        #endregion

        #region Private Methods (BST)






        private static void AddToList(IntervalTreeNode<TKey, TValue> node, List<RangeNode<TKey, TValue>> list)
        {
            if (node == null)
            {
                return;
            }

            AddToList(node.Left, list);

            list.AddRange(node.Values);

            AddToList(node.Right, list);
        }







        private IntervalTreeNode<TKey, TValue> GetNode(TKey key)
        {
            if (!typeof(TKey).IsValueType)
            {
                ArgumentNullException.ThrowIfNull(key);
            }

            IntervalTreeNode<TKey, TValue> node = Root;
            while (node != null)
            {
                int cmp = key.CompareTo(node.Start);
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








        private static void GetValues(IntervalTreeNode<TKey, TValue> node, TKey start, TKey end, ref TValue[] overlaps, ref int overlapCount)
        {
            if (node == null || start.CompareTo(node.Max) >= 0)
            {
                return;
            }

            GetValues(node.Left, start, end, ref overlaps, ref overlapCount);

            bool endsOnRight = end.CompareTo(node.Start) > 0;
            if (endsOnRight)
            {
                if (start.CompareTo(node.End) < 0)
                {

                    foreach (RangeNode<TKey, TValue> overlap in node.Values)
                    {
                        if (start.CompareTo(overlap.End) < 0)
                        {
                            if (overlaps.Length <= overlapCount)
                            {
                                Array.Resize(ref overlaps, overlapCount + ArrayGrowthSize);
                            }

                            overlaps[overlapCount++] = overlap.Value;
                        }
                    }
                }

                GetValues(node.Right, start, end, ref overlaps, ref overlapCount);
            }
        }







        private void Insert(TKey start, TKey end, TValue value)
        {
            IntervalTreeNode<TKey, TValue> newNode = BSTInsert(start, end, value);
            RestoreBalanceAfterInsertion(newNode);
        }






        private static void PropagateIncrease(IntervalTreeNode<TKey, TValue> node)
        {
            TKey max = node.Max;
            IntervalTreeNode<TKey, TValue> ptr = node;

            while ((ptr = ptr.Parent) != null)
            {
                if (max.CompareTo(ptr.Max) > 0)
                {
                    ptr.Max = max;
                }
                else
                {
                    break;
                }
            }
        }






        private static void PropagateFull(IntervalTreeNode<TKey, TValue> node)
        {
            IntervalTreeNode<TKey, TValue> ptr = node;

            do
            {
                TKey max = ptr.End;

                if (ptr.Left != null && ptr.Left.Max.CompareTo(max) > 0)
                {
                    max = ptr.Left.Max;
                }

                if (ptr.Right != null && ptr.Right.Max.CompareTo(max) > 0)
                {
                    max = ptr.Right.Max;
                }

                ptr.Max = max;
            } while ((ptr = ptr.Parent) != null);
        }











        private IntervalTreeNode<TKey, TValue> BSTInsert(TKey start, TKey end, TValue value)
        {
            IntervalTreeNode<TKey, TValue> parent = null;
            IntervalTreeNode<TKey, TValue> node = Root;

            while (node != null)
            {
                parent = node;
                int cmp = start.CompareTo(node.Start);
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
                    node.Values.Add(new RangeNode<TKey, TValue>(start, end, value));

                    if (end.CompareTo(node.End) > 0)
                    {
                        node.End = end;
                        if (end.CompareTo(node.Max) > 0)
                        {
                            node.Max = end;
                            PropagateIncrease(node);
                        }
                    }

                    Count++;
                    return node;
                }
            }

            IntervalTreeNode<TKey, TValue> newNode = new(start, end, value, parent);
            if (newNode.Parent == null)
            {
                Root = newNode;
            }
            else if (start.CompareTo(parent.Start) < 0)
            {
                parent.Left = newNode;
            }
            else
            {
                parent.Right = newNode;
            }

            PropagateIncrease(newNode);
            Count++;
            return newNode;
        }







        private int Delete(TKey key, TValue value)
        {
            IntervalTreeNode<TKey, TValue> nodeToDelete = GetNode(key);

            if (nodeToDelete == null)
            {
                return 0;
            }

            int removed = nodeToDelete.Values.RemoveAll(node => node.Value.Equals(value));

            if (nodeToDelete.Values.Count > 0)
            {
                if (removed > 0)
                {
                    nodeToDelete.End = nodeToDelete.Values.Max(node => node.End);


                    PropagateFull(nodeToDelete);
                }

                return removed;
            }

            IntervalTreeNode<TKey, TValue> replacementNode;

            if (LeftOf(nodeToDelete) == null || RightOf(nodeToDelete) == null)
            {
                replacementNode = nodeToDelete;
            }
            else
            {
                replacementNode = PredecessorOf(nodeToDelete);
            }

            IntervalTreeNode<TKey, TValue> tmp = LeftOf(replacementNode) ?? RightOf(replacementNode);

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
                nodeToDelete.Start = replacementNode.Start;
                nodeToDelete.Values = replacementNode.Values;
                nodeToDelete.End = replacementNode.End;
                nodeToDelete.Max = replacementNode.Max;
            }

            PropagateFull(replacementNode);

            if (tmp != null && ColorOf(replacementNode) == Black)
            {
                RestoreBalanceAfterRemoval(tmp);
            }

            return removed;
        }

        #endregion

        protected override void RotateLeft(IntervalTreeNode<TKey, TValue> node)
        {
            if (node != null)
            {
                base.RotateLeft(node);

                PropagateFull(node);
            }
        }

        protected override void RotateRight(IntervalTreeNode<TKey, TValue> node)
        {
            if (node != null)
            {
                base.RotateRight(node);

                PropagateFull(node);
            }
        }

        public bool ContainsKey(TKey key)
        {
            ArgumentNullException.ThrowIfNull(key);

            return GetNode(key) != null;
        }
    }






    public readonly struct RangeNode<TKey, TValue>
    {
        public readonly TKey Start;
        public readonly TKey End;
        public readonly TValue Value;

        public RangeNode(TKey start, TKey end, TValue value)
        {
            Start = start;
            End = end;
            Value = value;
        }
    }






    public class IntervalTreeNode<TKey, TValue> : IntrusiveRedBlackTreeNode<IntervalTreeNode<TKey, TValue>>
    {



        internal TKey Start;




        internal TKey End;




        internal TKey Max;




        internal List<RangeNode<TKey, TValue>> Values;

        internal IntervalTreeNode(TKey start, TKey end, TValue value, IntervalTreeNode<TKey, TValue> parent)
        {
            Start = start;
            End = end;
            Max = end;
            Values = [new RangeNode<TKey, TValue>(start, end, value)];
            Parent = parent;
        }
    }
}
