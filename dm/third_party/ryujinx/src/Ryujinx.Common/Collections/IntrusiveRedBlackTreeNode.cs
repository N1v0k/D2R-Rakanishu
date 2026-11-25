namespace Ryujinx.Common.Collections
{



    public class IntrusiveRedBlackTreeNode<T> where T : IntrusiveRedBlackTreeNode<T>
    {
        public bool Color = true;
        public T Left;
        public T Right;
        public T Parent;
        public T Predecessor;
        public T Successor;
    }
}
