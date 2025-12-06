pub struct SelfAwareIterator<T: Iterator> {
    iterator: T,
    exhausted: bool
}

impl<T: Iterator> SelfAwareIterator<T> {
    pub fn new(iterator: T) -> Self {
        SelfAwareIterator {
            iterator: iterator,
            exhausted: false
        }
    }

    pub fn is_exhausted(&self) -> bool {
        self.exhausted
    }
}

impl<T: Iterator> Iterator for SelfAwareIterator<T> {
    type Item = T::Item;

    fn next(&mut self) -> Option<Self::Item> {
        let result = self.iterator.next();
        if !self.exhausted && result.is_none() { self.exhausted = true; }
        result
    }
}
