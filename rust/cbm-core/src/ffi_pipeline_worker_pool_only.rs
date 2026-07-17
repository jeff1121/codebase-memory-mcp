#![allow(unsafe_code)]

//! `worker_pool.c` 的 Rust direct-only ABI 實作。
//!
//! 保留 C 的 callback、context、options layout 與 dispatch policy；每個 worker
//! 使用 8 MiB stack，主執行緒也會參與工作分派。

use core::ffi::{c_int, c_void};
use std::sync::atomic::{AtomicI32, Ordering};
use std::thread;

const WORKER_STACK_SIZE: usize = 8 * 1024 * 1024;
const MIN_WORKERS: c_int = 1;

type CbmParallelFn = unsafe extern "C" fn(c_int, *mut c_void);

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CbmParallelForOpts {
    pub max_workers: c_int,
    pub force_pthreads: u8,
}

unsafe extern "C" {
    fn cbm_default_worker_count(initial: bool) -> c_int;
}

fn run_serial(count: c_int, function: CbmParallelFn, context: usize) {
    for index in 0..count {
        unsafe { function(index, context as *mut c_void) };
    }
}

fn run_worker(count: c_int, function: CbmParallelFn, context: usize, next_index: &AtomicI32) {
    loop {
        let index = next_index.fetch_add(1, Ordering::Relaxed);
        if index >= count {
            break;
        }
        unsafe { function(index, context as *mut c_void) };
    }
}

fn run_threaded(count: c_int, function: CbmParallelFn, context: usize, worker_count: c_int) {
    let next_index = AtomicI32::new(0);
    thread::scope(|scope| {
        let mut handles = Vec::new();
        if handles.try_reserve(worker_count as usize).is_err() {
            run_serial(count, function, context);
            return;
        }

        for _ in 0..worker_count {
            let result = thread::Builder::new()
                .stack_size(WORKER_STACK_SIZE)
                .spawn_scoped(scope, || run_worker(count, function, context, &next_index));
            match result {
                Ok(handle) => handles.push(handle),
                Err(_) => break,
            }
        }

        run_worker(count, function, context, &next_index);
        for handle in handles {
            let _ = handle.join();
        }
    });
}

#[no_mangle]
/// 以 serial 或 scoped worker threads 執行 callback。
///
/// # Safety
/// `function` 必須是 null 或可呼叫的 C ABI callback；`context` 必須在整個
/// dispatch 期間保持有效，且 callback 必須能安全地由多個執行緒同時呼叫。
pub unsafe extern "C" fn cbm_parallel_for(
    count: c_int,
    function: Option<CbmParallelFn>,
    context: *mut c_void,
    opts: CbmParallelForOpts,
) {
    let Some(function) = function else {
        return;
    };
    if count <= 0 {
        return;
    }

    let mut worker_count = opts.max_workers;
    if worker_count <= 0 {
        worker_count = unsafe { cbm_default_worker_count(true) };
    }
    if worker_count < MIN_WORKERS {
        worker_count = MIN_WORKERS;
    }

    let context = context as usize;
    if worker_count <= MIN_WORKERS || count <= MIN_WORKERS {
        run_serial(count, function, context);
        return;
    }

    run_threaded(count, function, context, worker_count);
}
