mod self_aware_iterator;

use std::env;
use std::fs::File;
use std::io::{self, BufRead};
use std::path::Path;
use std::ops::RangeInclusive;
use range_set_blaze::RangeSetBlaze;
use self_aware_iterator::SelfAwareIterator;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 { // first argument is implicit (the path of the executable)
        println!("expected one argument, the input file");
        return;
    }

    if let Ok(lines) = read_lines(&args[1]) {
        let mut lines = SelfAwareIterator::new(lines);

        let fresh_ids = RangeSetBlaze::from_iter(
            lines.by_ref()
                .map_while(Result::ok)
                .take_while(|line| line.len() > 0) // list of ranges is terminated with an empty line
                .map_while(parse_id_range)
        );

        let mut fresh_available_ingredient_count: u32 = 0;
        for id in lines.by_ref()
            .map_while(Result::ok)
            .map_while(parse_id)
        {
            if fresh_ids.contains(id) {
                fresh_available_ingredient_count += 1;
            }
        }

        if !lines.is_exhausted() { // should have reached the end of the input file
            println!("failed to parse input");
            return;
        }

        println!("Number of available ingredients that are fresh: {}", fresh_available_ingredient_count);
    } else {
        println!("failed to open input file");
        return;
    }
}

fn parse_id_range(line: String) -> Option<RangeInclusive<u64>> {
    let (start, end) = line.split_once('-')?;
    let start_index = start.parse::<u64>().ok()?;
    let end_index = end.parse::<u64>().ok()?;
    Some(start_index..=end_index)
}

fn parse_id(line: String) -> Option<u64> {
    line.parse::<u64>().ok()
}

fn read_lines<P: AsRef<Path>>(path: P) -> io::Result<io::Lines<io::BufReader<File>>> {
    let file = File::open(path)?;
    Ok(io::BufReader::new(file).lines())
}
