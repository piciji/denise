// Copyright notice: The following tests are based on the 68k simulator from emoon. https://github.com/emoon/fx68k

// This is not an automated overall test, but many individual tests and serve more as a template.
// The tests of the individual instructions include what I tested last and thus only reflect the last test state of the respective instructions.
// The real point is to have to adjust as little as possible in another test.


#![allow(clippy::borrowed_box, clippy::transmute_ptr_to_ref)]
#![allow(unused_variables, unused_mut)]

use byteorder::{BigEndian, WriteBytesExt};
use std::ffi::c_void;

/// Holds all the CPU state data such as registers, flags, etc
#[repr(C)]
pub struct CpuState {
    /// data registers
    d_registers: [u32; 8],
    /// address registers
    a_registers: [u32; 8],
    /// program counter
    pc: u32,
    /// status flags
    flags: u32,
    /// last read address
    last_read_address: u32,
    /// last written address
    last_written_address: u32,
    
    read_val: u32,
    written_val: u32,
    fc: u8,
    ssp: u32,
    usp: u32,
    psw: u8,
    irc: u16,
    halted: u8,    
    reset: u8,    
    au:u32,
}

#[derive(Copy, Clone)]
pub struct CodeAdress(u32);

#[derive(Copy, Clone)]
pub struct StackAddress(u32);

#[derive(Copy, Clone, PartialEq)]
pub enum Register {
    Data(u8),
    Address(u8),
}

extern "C" {
    fn fx68k_ver_new_instance(memory_interface: *mut c_void) -> *mut c_void;
    fn fx68k_ver_step_cycle(context: *mut c_void);
    fn fx68k_setIPL(context: *mut c_void, ipl: u8);
    fn fx68k_ver_cpu_state(context: *mut c_void) -> CpuState;
    fn fx68k_update_memory(context: *mut c_void, address: u32, data: *const c_void, length: u32);
    fn fx68k_update_register(context: *mut c_void, register: u32, value: u32);
    fn fx68k_set_imask(context: *mut c_void, mask: u8);
    fn fx68k_get_microlatch(context: *mut c_void) -> u32;
}

pub struct Fx68k {
    /// private ffi instance
    ffi_instance: *mut c_void,
}

pub trait MemoryInterface {
    /// read u8 value from memory address. cycle is the current CPU cycle the read is being executed on.
    fn read_u8(&mut self, cycle: u32, address: u32) -> Option<u8>;
    /// read u16 value from memory address. cycle is the current CPU cycle the read is being executed on.
    fn read_u16(&mut self, cycle: u32, address: u32) -> Option<u16>;
    /// write u8 value to memory address. cycle is the current CPU cycle the write is being executed on.
    fn write_u8(&mut self, cycle: u32, address: u32, value: u8) -> Option<()>;
    /// write u16 value to memory address. cycle is the current CPU cycle the write is being executed on.
    fn write_u16(&mut self, cycle: u32, address: u32, value: u16) -> Option<()>;
}

#[no_mangle]
pub unsafe extern "C" fn fx68k_mem_read_u8(
    context: *mut Box<dyn MemoryInterface>,
    cycle: u32,
    address: u32,
) -> u8 {
    //println!("reading fx68k_mem_read_u8");
    let cb: &mut Box<dyn MemoryInterface> = std::mem::transmute(context);
    cb.read_u8(cycle, address).unwrap()
}

#[no_mangle]
pub unsafe extern "C" fn fx68k_mem_read_u16(
    context: *mut Box<dyn MemoryInterface>,
    cycle: u32,
    address: u32,
) -> u16 {
    //println!("reading fx68k_mem_read_u16 {} {}", cycle, address);
    let cb: &mut Box<dyn MemoryInterface> = std::mem::transmute(context);
    cb.read_u16(cycle, address).unwrap()
}

#[no_mangle]
pub unsafe extern "C" fn fx68k_mem_write_u8(
    context: *mut Box<dyn MemoryInterface>,
    cycle: u32,
    address: u32,
    value: u8,
) {
    let cb: &mut Box<dyn MemoryInterface> = std::mem::transmute(context);
    cb.write_u8(cycle, address, value).unwrap()
}

#[no_mangle]
pub unsafe extern "C" fn fx68k_mem_write_u16(
    context: *mut Box<dyn MemoryInterface>,
    cycle: u32,
    address: u32,
    value: u16,
) {
    let cb: &mut Box<dyn MemoryInterface> = std::mem::transmute(context);
    cb.write_u16(cycle, address, value).unwrap()
}

impl Fx68k {
    /// Create a new instance with a memory interface. Using this the CPU will boot up and read
    /// from address 0 and 4 to fetch the stack pointer and where to start code execution so it's
    /// up to the memory implementation to have this data correct.
    pub fn new_with_memory_interface<T: MemoryInterface>(memory_interface: T) -> Fx68k {
        unsafe {
            let f: Box<Box<dyn MemoryInterface>> = Box::new(Box::new(memory_interface));
            let memory_interface = Box::into_raw(f) as *mut _;

            Fx68k {
                ffi_instance: fx68k_ver_new_instance(memory_interface),
            }
        }
    }

    /// Create a new instance with some m68k code. Memory will be allocated to the size of
    /// memory_size, the CPU will boot up using this memory with code_address an stack_address set
    /// with the start_address and stack_address being setup. After the boot up is complete the
    /// data in code will be copied to the location of code_start_address in the memory.
    pub fn new_with_code(
        code: &[u8],
        code_address: CodeAdress,
        stack_address: StackAddress,
        memory_size: usize,
    ) -> Fx68k {
        let mut data = vec![0; memory_size];

        // Write the initial startup data
        //data.write_u32::<BigEndian>(code_address.0).unwrap();
        //data.write_u32::<BigEndian>(stack_address.0).unwrap();

        // setup the core
        let mut core = Fx68k::new_with_memory_interface(Fx68kVecMemoryInterface::new(data));


core.update_memory(0, &[0,0,0,52,0,0,0,0]);
//core.update_memory(0, &[0,0,0,52,0, 0x01,0xfe,0xfe]);

        // this will boot the core, setup the new PC and stack pointer
        core.boot();

        // write the code to the ram memory
        core.update_memory(0, code);
                

       // core.update_memory(0x1fefe, &[0x61, 0, 1, 5, 0x80, 0xc0]);

        // and finished
        core
    }
// http://logout.hotspots
    /// Boot up the core. Run for n number of cycles and expect pc being 0, 4 and then something
    /// else (depending on the data in memory) if this ends up not being true the boot of the core
    /// has failed and this function will return false otherwise true.
    pub fn boot(&mut self) {
        // this is a kinda ugly hard coded value, but will do for now
        for _ in 0..240 {
            self.step();
        }
    }

    /// Updated memory the memory for a core
    pub fn update_memory(&mut self, address: u32, data: &[u8]) {
        unsafe {
            fx68k_update_memory(
                self.ffi_instance,
                address,
                data.as_ptr() as *const c_void,
                data.len() as u32,
            );
        }
    }

    /// step the CPU one cycle (notice that this doesn't step one CPU cycle but a cycle with the
    /// timer that the 68k CPU requires (see the user manual for more info) and step_cpu_cycle to
    /// step with the CPU timing clock
    pub fn step(&mut self) {
        unsafe {
            fx68k_ver_step_cycle(self.ffi_instance);
        }
    }

    pub fn get_microlatch(&mut self) -> u32 {
        unsafe {
            return fx68k_get_microlatch(self.ffi_instance);
        }
    }
    
    pub fn set_imask(&mut self, mask: u8) {
        unsafe {
            fx68k_set_imask(self.ffi_instance, mask);
        }
    }
    
    pub fn setIPL(&mut self, ipl: u8) {
        unsafe {
            fx68k_setIPL(self.ffi_instance, ipl);
        }
    }

    /// step one CPU instruction. In this context it means stepping the CPU until the PC has
    /// changed. Return value is number of cycles that was needed for the PC to change. If PC
    /// wasn't able to change in 10000 steps None will be returned
    pub fn step_instruction(&mut self) -> Option<usize> {
        let prev_pc = self.cpu_state().pc;

        for i in 0..10000 {
            self.step();

            if self.cpu_state().pc != prev_pc {
                return Some(i);
            }
        }

        None
    }

    /// Run until reached a certain PC and returns number of cycles it took
    pub fn run_until(&mut self, pc: u32) -> usize {
        let mut total_cycle = 0;

        loop {
            total_cycle += self.step_instruction().unwrap();

            if self.cpu_state().pc == pc {
                return total_cycle;
            }
        }
    }

    /// Get the current state of the CPU (registers, pc, flags, etc)
    pub fn cpu_state(&self) -> CpuState {
        unsafe { fx68k_ver_cpu_state(self.ffi_instance) }
    }

    /// Update a register for the CPU
    pub fn set_register(&mut self, register: Register, data: u32) {
        unsafe {
            match register {
                Register::Data(reg) => {
                    fx68k_update_register(self.ffi_instance, u32::from(reg), data)
                }
                Register::Address(reg) => {
                    fx68k_update_register(self.ffi_instance, u32::from(reg + 8), data)
                }
            }
        }
    }
}

pub struct Fx68kVecMemoryInterface {
    data: Vec<u8>,
}

impl Fx68kVecMemoryInterface {
    pub fn new(data: Vec<u8>) -> Fx68kVecMemoryInterface {
        Fx68kVecMemoryInterface { data: data.clone() }
    }
}

impl MemoryInterface for Fx68kVecMemoryInterface {
    fn read_u8(&mut self, _cycle: u32, address: u32) -> Option<u8> {
        Some(self.data[address as usize])
    }

    fn read_u16(&mut self, _cycle: u32, address: u32) -> Option<u16> {
        let v0 = u16::from(self.data[address as usize]);
        let v1 = u16::from(self.data[address as usize + 1]);
       // println!("reading fx68k_mem_read_u16 {} {}", v0, v1);
        Some((v0 << 8) | v1)
    }

    fn write_u8(&mut self, _cycle: u32, address: u32, value: u8) -> Option<()> {
        self.data[address as usize] = value;
        Some(())
    }

    fn write_u16(&mut self, _cycle: u32, address: u32, value: u16) -> Option<()> {
        self.data[address as usize] = (value >> 8) as u8;
        self.data[address as usize + 1] = (value & 0xff) as u8;
        Some(())
    }
}

#[cfg(test)]
#[allow(dead_code)]
#[allow(unused_variables, unused_mut)]
mod tests {
    use super::*;

   // #[test]
    fn test_read_boot_ok() {
        let mut data = vec![];
        // Write the initial startup data
        data.write_u32::<BigEndian>(0).unwrap();
        data.write_u32::<BigEndian>(8).unwrap();

        let mut core = Fx68k::new_with_memory_interface(Fx68kVecMemoryInterface::new(data));
        core.boot();

        let state = core.cpu_state();

        // make sure last address read was
        assert_eq!(state.last_read_address, 6);
    }

    //#[test]
    fn test_new_code() {
        // create a core with a nop instruction and step it
        let mut _core = Fx68k::new_with_code(&[0x4e, 0x71], CodeAdress(0), StackAddress(0), 16);
    }

  //  #[test]
	fn test_reset_timing() {
		// move.B Dn -> (D16,An)
		let mut core = Fx68k::new_with_code(&[0x23, 0x40, 0x00, 0x00, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 16);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 12);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		
                        
		// full prefetch
		for _ in 0..96 {
            //core.step();
        }	
        // 12
        for _ in 0..96 {
            //core.step();
        }		
        // 30
        for _ in 0..64 {
            //core.step();
        }	
        
        
		let mlatch = core.get_microlatch();
		assert_eq!(14, mlatch);
		
	}
    
//	#[test]
	fn test_move_d16_an() {
		// move.B Dn -> (D16,An)
		let mut core = Fx68k::new_with_code(&[0x23, 0x40, 0x00, 0x00, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 16);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 12);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }		
        
        let steps = 32 + 32 + 0;
        
        for _ in 0 .. steps {
            core.step();
		}
		        
        core.setIPL(7);
        
	    let steps =  0 + 30 + 32 + 20  * 8 ;                
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		assert_eq!(14, core.cpu_state().d_registers[1]);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}
    
	//#[test]
	fn test_move_dn() {
		// move.B Dn -> Dn
		let mut core = Fx68k::new_with_code(&[0x12, 0x00, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 16);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 12);
		core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..56 {
            core.step();
        }
			
		
		core.setIPL(7);
		
		for _ in 0..8 {
            core.step();
        }
		
	    let steps =  0 + 32 + 0 + 20  * 8 ;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		assert_eq!(14, core.cpu_state().d_registers[1]);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}

    //#[test]
	fn test_reset_move() {
		// move.B (A0) -> (A1)
		let mut core = Fx68k::new_with_code(&[0x32, 0x90, 0x80, 0xc0, 0x4e, 0x71,  0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71, 0x1, 0x0, 0x0, 0x3], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 7);
		core.set_register(Register::Address(1), 10);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
        
		//let steps = 52 * 8 + 0;  // 12 (2 + 10) + 2, 22 for halted
		let steps = (52) * 8 + 0;
		for _ in 0..steps {
            core.step();
        }

		let state = core.cpu_state();
		
		 println!("ra {} rv {} wa {} wv {} fc {} halt {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.fc, state.halted);
		
		//for _ in 0..32 {
      //      core.step();
        //}
	//	let state = core.cpu_state();
	
	//	assert_eq!(14, core.cpu_state().d_registers[1]);
	//	assert_eq!(2, state.pc);
		//assert_eq!(10, state.last_read_address);
		//assert_eq!(10, state.pc);
	assert_eq!(10, 1);
	//assert_eq!(10, state.fc);
	//assert_eq!(10, state.halted);
		//assert_eq!(12, state.last_written_address);		
		//assert_eq!(10, state.written_val);
		//assert_eq!(10, state.ssp);
	}
	
		//#[test]
	fn test_addi() {
		// addi.L -> (An) 
		let mut core = Fx68k::new_with_code(&[0x06, 0x90, 0, 3, 0, 3, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 82);
		core.set_register(Register::Address(0), 8);
		core.set_register(Register::Address(6), 0x12345678);
		core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

		
	    let steps =  (8 + 4 + 3)  * 8 + 3;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		
		//let steps =  0 + 30 + 20  * 8 ;                
		let steps = 5 + (4 + 3) * 8 + 4;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} d0 {} d1 {}  fc {} halt {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.d_registers[0], state.d_registers[1], state.fc, state.halted);
		assert_eq!(14, 1);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}
	
	  //  #[test]
	fn test_move_retrigger_ipl7() {
		// move.B (An) -> (An)
		let mut core = Fx68k::new_with_code(&[0x46, 0xc2, 0x32, 0x90, 0x46, 0xc3, 0x32, 0x90,  0x46, 0xc2,   0x80, 0xc0, 0x4e, 0x71, 0x5e, 0x71,    0x0, 0x0, 0x0, 0x4,   0,0,0,31], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 300);
		core.set_register(Register::Address(1), 320);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
        core.set_register(Register::Data(2), 0x2300);       
        core.set_register(Register::Data(3), 0x2700);       
        core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
        
		// set imask to 3
        for _ in 0..96 {
            core.step();
        }
        
        core.setIPL(7);
        
        //move
        for _ in 0..96 {
            core.step();
        }
                
        // level 7 interrupt
		let steps =  (6 + 4 + 8 + 8 + 8 + 4 + 2 + 4) * 8 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		// set imask to 7
        for _ in 0..96 {
            core.step();
        }
			
		// move + massk to 3 (retrigger level 7, even there is no transition)
		let steps = (12 + 12 + 6 + 4 + 3) * 8 + 5;
		
		for _ in 0 .. steps {
            core.step();
		}
			/*	
		core.setIPL(4);
		
	    let steps =  10 + 32 + 32 + (6 + 4 + (4 + 4) + 4 + 4 + 8 + 12 + 36 + 0) * 8;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(5);
		
		let steps =  (4 + 2 + 4 + 6 + 4) * 8;
		
		for _ in 0 .. steps {
            core.step();
		}

		*/
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {}  fc {} halt {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.fc, state.halted);
	//	assert_eq!(2, state.pc);
		assert_eq!(10, 1);
		//assert_eq!(12, state.last_written_address);		
	}
    
	//#[test]
	fn test_move_xxxw() {
		// move.B (xxx).W -> Dn
		let mut core = Fx68k::new_with_code(&[0x12, 0x38, 0x00, 0x00, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 16);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 12);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
			
			
		let steps = 32 + 24 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		
	    let steps =  32 + 8 + 20  * 8 ;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
	//	assert_eq!(2, state.pc);
		assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}

//	#[test]
	fn test_move_an() {
		// move.B (A0) -> (A1)
		let mut core = Fx68k::new_with_code(&[0x12, 0x90, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20,0,1,  1,0,0,3], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 12);
		core.set_register(Register::Data(0), 14);
		
			//let state = core.cpu_state();
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
			
			
		let steps = 32 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		
	    let steps =  32 + 32 + (6 + 4 + 4 + 4 + 4 + 4 + 8 + 12 + 20 + 3) * 8 + 5;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {}  fc {} halt {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.fc, state.halted);
		
	//	assert_eq!(2, state.pc);
		//assert_eq!(10, state.last_read_address);
		assert_eq!(13,1);		
	}
	
		//		#[test]
	fn test_line_a() {
		// line a
		let mut core = Fx68k::new_with_code(&[0xa0, 0x00, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0, 0,0,0,0,0,0,0,0,0,0,0,  0,0,0,7], CodeAdress(2), StackAddress(0), 550);
		//core.set_register(Register::Address(0), 28);
	//	core.set_register(Register::Address(1), 26);
	//	core.set_register(Register::Address(7), 7);
		core.set_register(Register::Data(0), 0x8000);
		core.set_register(Register::Data(1), 0x8001);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

	    //let steps =  16 + 32 + 32 + 32 + 32 + 32 + 32 + 32 + 32 + 16 + 32 + 20;#
		let steps = (4  + 12 + 8 + 12 + 16  + 4+ 3) * 8 + 5;
		for _ in 0 .. steps {
      //      core.step();
		}
		
		//core.setIPL(7);
		let state = core.cpu_state();
		
	    let steps = ( 48 + 3 ) * 8 + 5;
		
		for _ in 0 .. steps {
      //      core.step();
		}
		
		//let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.fc, state.halted, state.pc);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
			
		//assert_eq!(13, state.written_val);
		//assert_eq!(13, state.last_written_address);		
		//assert_eq!(101, state.ssp);
		assert_eq!(101, 1);
		//assert_eq!(101, state.a_registers[7]);
	}

	//			#[test]
	fn test_sr_priv() {
		// sr priv
		let mut core = Fx68k::new_with_code(&[0x46, 0xc2, 0x02, 0x7c, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,9,0,9,0, 9,0,0,  0,9,0,9,0,0,0,0,  0,0,0,7], CodeAdress(2), StackAddress(0), 550);
		//core.set_register(Register::Address(0), 28);
	//	core.set_register(Register::Address(1), 26);
	//	core.set_register(Register::Address(7), 7);
		core.set_register(Register::Data(0), 0x8000);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(2), 0x700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			
        
		for _ in 0..96 { // set priv to user
            core.step();
        }		

	    //let steps =  16 + 32 + 32 + 32 + 32 + 32 + 32 + 32 + 32 + 16 + 32 + 20;#
		let steps = (4  + 12 + 8 + 12 + 20 + 3) * 8 + 5;
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(7);
		let state = core.cpu_state();
		
	    let steps = ( 48 + 3 ) * 8 + 5;
		
		for _ in 0 .. steps {
      //      core.step();
		}
		
		//let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {}  fc {} halt {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.fc, state.halted);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
			
		//assert_eq!(13, state.written_val);
		//assert_eq!(13, state.last_written_address);		
		//assert_eq!(101, state.ssp);
		assert_eq!(101, 1);
		//assert_eq!(101, state.a_registers[7]);
	}

   // #[test]
    fn test_two_nop_instructions() {
        // create a core and run moveq #100,d0
        let mut core = Fx68k::new_with_code(
            &[0x4e, 0x71, 0x4e, 0x71],
            CodeAdress(0),
            StackAddress(0),
            16,
        );
        core.step_instruction();
        core.step_instruction();
    }

   // #[test]
    fn test_divs_instructions() {
        // create a core and two divs #2,d0 instructions and update the d0 register before running
        let mut core = Fx68k::new_with_code(
            &[0x81, 0xfc, 0x00, 0x02, 0x81, 0xfc, 0x00, 0x02, 0x4e, 0x71],
            CodeAdress(0),
            StackAddress(0),
            16,
        );
        core.set_register(Register::Data(0), 8);

        core.run_until(8);
        assert_eq!(4, core.cpu_state().d_registers[0]);

        core.run_until(12);
        assert_eq!(2, core.cpu_state().d_registers[0]);
    }   
	
	  //  #[test]
	fn test_reset() {
		// reset
		let mut core = Fx68k::new_with_code(&[0x4e,0x70, 0x80, 0xc0, 0x4e, 0xb9, 0,0,0,6, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 82);
		core.set_register(Register::Address(0), 14);
		core.set_register(Register::Address(1), 35);
		core.set_register(Register::Address(6), 0x12345678);
		core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 14);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

		
	  //  let steps =  (4 + 12 +   4 + 12 + 20 +  3)  * 8 + 5;
	  let steps =  (124)  * 8 + 0;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		
	    let steps = ( 8  ) * 8 + 0 + 10 * 8;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} d0 {} d1 {}  fc {} halt {} reset {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.d_registers[0], state.d_registers[1], state.fc, state.halted, state.reset);
		assert_eq!(14, 1);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}
	
	//		    #[test]
	fn test_nmi_short_pulse() {
		// nmi short pulse
		let mut core = Fx68k::new_with_code(&[0x48, 0x10,  0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e,0x71, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0,0,0,0, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 14);
		core.set_register(Register::Address(1), 35);
		core.set_register(Register::Address(6), 0x12345678);
		core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 0x2300);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		core.setIPL(7);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

		
	  //  let steps =  (4 + 12 +   4 + 12 + 20 +  3)  * 8 + 5;
	  let steps =  (6 + 4 + 8 + 8    + 8 + 2 + 8 + 4 + 6)  * 8 + 0;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(4);
		
		let steps =  (1)  * 8 + 0;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		
	    let steps = ( 1 + 0 +   4 + 6 + 2  ) * 8 + 5 ;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} d0 {} d1 {}  fc {} halt {} pc {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.d_registers[0], state.d_registers[1], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}
	
//				    #[test]
	fn test_move_double_sample() {
		// double sample for move.w DN, (d16,An)
		let mut core = Fx68k::new_with_code(&[0x46,0xc0, 0x31, 0x40,  0x0, 0x2, 0x80, 0xc0, 0x4e, 0x71, 0x4e,0x71, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0,0,0,0, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 14);
		core.set_register(Register::Address(1), 35);
		core.set_register(Register::Address(6), 0x12345678);
	//	core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 0x0300);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	  let steps =  (12 + 0 + 0)  * 8 + 0;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(5);
		
		let steps =  (4+ 0)  * 8 + 0;
	 	
		for _ in 0 .. steps {
     //       core.step();
		}
		
	//	core.setIPL(6);
		
	    let steps = ( 4 + 2 + 4 +    6 + 4 + 3  ) * 8 + 5 ;
		
		for _ in 0 .. steps {
      //      core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} usp {} d0 {} d1 {} a7 {}  fc {} halt {} pc {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.usp, state.d_registers[0], state.d_registers[1], state.a_registers[7], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);
	//	assert_eq!(2, state.pc);
	//	assert_eq!(10, state.last_read_address);
		//assert_eq!(12, state.last_written_address);		
	}
/////////////////////////////////////////////////////////////
	
	//#[test]
	fn test_bcc() { 
		let mut core = Fx68k::new_with_code(&[0x60, 0x0 , 0, 7, 0x80, 0xc0, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		//core.set_register(Register::Address(0), 28);
	//	core.set_register(Register::Address(1), 26);
	//	core.set_register(Register::Address(7), 7);
		core.set_register(Register::Data(0), 0x8000);
		core.set_register(Register::Data(1), 0x8001);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = (2 + 12 +  3) * 8 + 5;
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(7);
		
	    let steps = ( 16 ) * 8 + 5;
		
		for _ in 0 .. steps {
       //    core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp{}  usp {} d0 {} d1 {} a7 {}  fc {} halt {} pc {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.d_registers[1], state.a_registers[7], state.fc, state.halted, state.pc);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_bsr() {
		// 0x4e, 0x61 (USP A1)  0x46, 0xc0 (SR <- D0)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc0, 0x61, 0x00, 0xff, 0xff, 0x80, 0xc0,  0x80, 0xc0, 0x4e, 0x71, 0x4e,0x71, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0,0,0,0, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 14);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(6), 0x12345678);
	//	core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 0x0700);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	  let steps =  (4+ 12 + 2 + 8 + 12  + 0+ 3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 0)  * 8 + 0;
	 	
		for _ in 0 .. steps {
          //  core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} usp {} d0 {} d1 {} a7 {}  fc {} halt {} pc {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.usp, state.d_registers[0], state.d_registers[1], state.a_registers[7], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_dbcc() {
		let mut core = Fx68k::new_with_code(&[0x51, 0xc9, 0, 6, 0x80, 0xc0,  0x80, 0xc0, 0x80, 0xc0, 0x4e,0x71, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0,0,0,0, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 14);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(6), 0x12345678);
	//	core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(1), 0x80800000);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(4), 0x700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	  let steps =  (2+4+3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (20+ 0)  * 8 + 0;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} usp {} d0 {} d1 {} a7 {}  fc {} halt {} pc {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.usp, state.d_registers[0], state.d_registers[1], state.a_registers[7], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_jmp() {
		let mut core = Fx68k::new_with_code(&[0x4e, 0xf8, 0, 7, 0x80, 0xc0,  0x80, 0xc0, 0x80, 0xc0, 0x4e,0x71, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0,0,0,0, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 11);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(6), 0x12345678);
	//	core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(1), 0x80800000);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(4), 0x700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	  let steps =  (4+12+ 0+ 3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
	//	core.setIPL(7);
		let steps =  (16+ 0)  * 8 + 0;
	 	
		for _ in 0 .. steps {
      //      core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} usp {} d0 {} d1 {} a7 {}  fc {} halt {} pc {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.usp, state.d_registers[0], state.d_registers[1], state.a_registers[7], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_jsr() {
		//  0x4e, 0x61 (USP A1)  0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e,0x61, 0x46, 0xc4, 0x4e, 0x90, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x80, 0xc0, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 82);
		core.set_register(Register::Address(0), 5);
		core.set_register(Register::Address(1), 35);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 4);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x0700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
        
        let steps =  (4+12 +  0+   0+ 0 +0+   12+ 0+ 3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}

//		core.setIPL(7);
		let steps =  (16+ 0)  * 8 + 5;
	 	
		for _ in 0 .. steps {
       //    core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_lea() {
		let mut core = Fx68k::new_with_code(&[0x4b, 0xf9, 0x12, 0x34, 0x56, 0x78, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x80, 0xc0, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 82);
		core.set_register(Register::Address(0), 5);
		core.set_register(Register::Address(1), 35);
		core.set_register(Register::Address(6), 0x12345678);
	//	core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 4);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x0700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
        
        let steps =  (4+ 3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}

		core.setIPL(7);
		let steps =  (16+ 0)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_pea() {
		//  0x4e, 0x61 (USP A1)  0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e,0x61, 0x46, 0xc4, 0x48, 0x50, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x80, 0xc0, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71], CodeAdress(0), StackAddress(0), 82);
		core.set_register(Register::Address(0), 6);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 6);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x0700);
		core.setIPL(0);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }
        
        let steps =  (4+11+  0+0+0+0+0+  0+0+ 0)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}

		core.setIPL(7);
		let steps =  (16+ 10)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_movem_reg() {
		let mut core = Fx68k::new_with_code(&[0x4c, 0x98, 0, 2, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 8);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

		
	    let steps =  (4 + 0 + 0+ 0 + 0 + 4 +  4 + 3)  * 8 + 5;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          //  core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//	#[test]
	fn test_movem_mem() {
		let mut core = Fx68k::new_with_code(&[0x48, 0xb9, 0, 3, 0,0,0, 10, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }


	    let steps =  (4+ 4+ 4 +4+ 4 + 0 + 0 +  0 + 3)  * 8 + 5;
		
		for _ in 0 .. steps {
            core.step();
		}
		
	//	core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
       //     core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//	#[test]
	fn test_subx_reg() {
		let mut core = Fx68k::new_with_code(&[0x93, 0x80, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }


	    let steps =  (0)  * 8 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_abcd_reg() {
		let mut core = Fx68k::new_with_code(&[0xc3, 0x00, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }


	    let steps =  (0)  * 8 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_addx_mem() {
		// 0x4e, 0x61 (USP A1)  0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc4,  0xd7, 0x8F, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 11);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(3), 22);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	    let steps =  (4 +12 +    2 + 4+ 4 + 4+  3)  * 8 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_cmpm() {
		//  0x4e, 0x61 (USP A1)  0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc4,  0xbf, 0x8b, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 11);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(3), 22);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	    let steps =  (4 +12  + 4 + 0 + 0 +0+  3)  * 8 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_andi_ccr() {
		let mut core = Fx68k::new_with_code(&[0x02, 0x3c, 0, 10, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 11);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(3), 22);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	    let steps =  (4 + 4 +4  + 0 + 0 + 0 +0+  3)  * 8 + 0;
		
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_andi_sr() {
		let mut core = Fx68k::new_with_code(&[0x02, 0x7c, 0, 0, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 11);
		core.set_register(Register::Address(1), 34);
		core.set_register(Register::Address(3), 22);
		core.set_register(Register::Address(6), 0x12345678);
		//core.set_register(Register::Address(7), 1000);
		core.set_register(Register::Data(0), 2);
		core.set_register(Register::Data(1), 24);
		core.set_register(Register::Data(4), 0x700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }

	    let steps =  (4 + 4 +4  + 0 + 0 + 0 +0+  3)  * 8 + 5;
		
		for _ in 0 .. steps {
            core.step();
		}
		
	//	core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           // core.step();
		}
		
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_chk() {
		let mut core = Fx68k::new_with_code(&[0x41, 0xa9, 0x0, 0x3, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		//core.set_register(Register::Address(0), 28);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 7);
		core.set_register(Register::Data(0), 0x80000001);
		core.set_register(Register::Data(1), 0x8001);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = (4 + 12 + 3+ 16) * 8 + 5;
		for _ in 0 .. steps {
            core.step();
		}
		
	//	core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
      //      core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_move_from_sr() {
		// 0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x46, 0xc4, 0x40, 0xf0, 0,2, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 7);
		core.set_register(Register::Data(0), 0x4);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0xffff);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = (12 + 2 + 4 + 4 + 4 +  3) * 8 + 5;
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          //  core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_move_to_sr() {
		let mut core = Fx68k::new_with_code(&[0x46, 0xf9, 0,0,0,6, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 7);
		core.set_register(Register::Data(0), 0x4);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0xffff);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 4 + 4 + 4+ 3) * 8 + 5;
		for _ in 0 .. steps {
            core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          //  core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {} a3 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.a_registers[3], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_exg() {
		let mut core = Fx68k::new_with_code(&[0xc1, 0x8f, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
		core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x10);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0xffff);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 6) * 8 + 5;
		for _ in 0 .. steps {
    //       core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
            core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_ext() {
		let mut core = Fx68k::new_with_code(&[0x48, 0xc0, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
		core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x12348678);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0xffff);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4) * 8 + 5;
		for _ in 0 .. steps {
          core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          // core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_rte() {
		let mut core = Fx68k::new_with_code(&[0x4e, 0x73, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x0, 0x0, 0x0, 0x8, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
		core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x12348678);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0xffff);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 4+4 + 0 +0+ 3) * 8 + 0;
		for _ in 0 .. steps {
          core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          // core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_rtr() {
		// 0x4e, 0x61 (USP A1)  0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc4, 0x4e, 0x77, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x0, 0x0, 0x0, 0x8, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 35);
		core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x12348678);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 +12   + 12 + 16+ 3 ) * 8 + 5;
		for _ in 0 .. steps {
          core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          // core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
		//#[test]
	fn test_rts() {
		let mut core = Fx68k::new_with_code(&[0x4e, 0x75, 0x80, 0xc0, 0x0, 0x3, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x0, 0x0, 0x0, 0x9, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
		core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x12348678);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0xffff);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 4+12 + 0 +0+ 3) * 8 + 5;
		for _ in 0 .. steps {
          core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          // core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.flags);
		assert_eq!(101, 1);
	}
	
	//#[test]
	fn test_stop() {
		// 0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[/*0x46, 0xc4,*/ 0x4e,0x72, 0x87, 0x00, 0x80, 0xc0,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
		core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x2700);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 4 + 4 + 4 + 3   /*  12 + 8 + 2 + 8 + 4 + 0*/) * 8 + 5;
		for _ in 0 .. steps {
          core.step();
		}
		
	//	core.setIPL(7);
		let steps =  (0 + 0 + 0 + 0  /*12 + 8 + 2 + 8 + 2*/)  * 8 + 5;
	 	
		for _ in 0 .. steps {
    //       core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
		//#[test]
	fn test_swap() {
		let mut core = Fx68k::new_with_code(&[0x48,0x40, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x2700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 2 ) * 8 + 5;
		for _ in 0 .. steps {
      //    core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_trap() {
		// 0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x46, 0xc4, 0x4e,0x41, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x2700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 12 + 4 + 12 + 8 + 3 ) * 8 + 5;
		for _ in 0 .. steps {
          core.step();
		}
		
		//core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          // core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_trapv() {
		// 0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x46, 0xc4, 0x4e,0x76, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x271f);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 12 + 4 + 3  ) * 8 + 5;
		for _ in 0 .. steps {
          core.step();
		}
		
	//	core.setIPL(7);
		let steps =  (1 + 4 + 6 + 3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           //core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_link() {
		// 0x4e, 0x61 (USP A1) 0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc4, 0x4e,0x50, 0, 12, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0x12345678);
		core.set_register(Register::Address(1), 30);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x0700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 12 + 3 ) * 8 + 0;
		for _ in 0 .. steps {
          core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_unlink() {
		// 0x4e, 0x61 (USP A1) 0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc4, 0x4e,0x58, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0,   0x12, 0x34, 0x56, 0x78, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 14);
		core.set_register(Register::Address(1), 27);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x0700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 12 + 0+0+   0 + 0 + 3 ) * 8 + 0;
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_movep_reg() {
		let mut core = Fx68k::new_with_code(&[0x01,0x48,  0,3,  0x8a, 0xc5,    0x4e,   0x12,0x34,0x56, 0x67, 0xc0,   0x12, 0x34, 0x56, 0x78, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 4);
		core.set_register(Register::Address(1), 27);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x80011234);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x0700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 + 4 + 4 + 3 ) * 8 + 0;
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_movep_mem() {
		let mut core = Fx68k::new_with_code(&[0x01,0xc8,  0,3,  0x8a, 0xc5,    0x4e,   0x12,0x34,0x56, 0x67, 0xc0,   0x12, 0x34, 0x56, 0x78, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 4);
		core.set_register(Register::Address(1), 27);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x80011234);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x0700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4+ 4+4+4+ 3 ) * 8 + 0;
		for _ in 0 .. steps {
            core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_lsl_imm() {
		let mut core = Fx68k::new_with_code(&[0xef,0x88, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x2700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..56 {
            core.step();
        }			

		let steps = ( 0 ) * 8 + 0;
		for _ in 0 .. steps {
          core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12 + 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_lsl_reg() {
		let mut core = Fx68k::new_with_code(&[0xe5,0xa8, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 0);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(2), 0xff);
		core.set_register(Register::Data(4), 0x2700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..56 {
            core.step();
        }			

		let steps = ( 0 ) * 8 + 0;
		for _ in 0 .. steps {
          core.step();
		}
		
		core.setIPL(7);
		let steps =  ( 1 + 4 + 4 + 126  + 6+ 3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
	//#[test]
	fn test_lsl_ea() {
		let mut core = Fx68k::new_with_code(&[0xe3,0xe0, 0x8a, 0xc5,    0x4e,0x71,0x4e,0x71, 0x80, 0xc0, 0x0, 0, 0x0, 8, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x21, 0x80, 0xc0, 0x4e, 0x71,   0,0,0,0,0,0,0,10 ,   0, 0, 0, 0x8, 0x4e, 0x71    ], CodeAdress(0), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 12);
		core.set_register(Register::Address(1), 4);
	//	core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x8001);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x2700);
		core.set_register(Register::Data(5), 0x8000);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 2 + 3 ) * 8 + 0;
		for _ in 0 .. steps {
          core.step();
		}
		
		core.setIPL(7);
		let steps =  (1 + 4 + 4 + 6 + 4 + 8 + 8 + 8 +12+ 20 +   3)  * 8 + 5;
	 	
		for _ in 0 .. steps {
           core.step();
		}
		let state = core.cpu_state();
		
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(14, 1);	
	}
	
    //#[test]
	fn test_move_minus_an() {
		// 0x4e, 0x61 (USP A1)  0x46, 0xc4 (SR <- D4)
		let mut core = Fx68k::new_with_code(&[0x4e, 0x61, 0x46, 0xc4, 0x2f, 0x10, 0x8a, 0xc5, 0x0, 0x3, 0x4e, 0x71, 0x80, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x0, 0x0, 0x0, 0x8, 0x4e, 0x71,   0x4e, 0x71,0,0,1,2,0,0,0,0,0,0,0,0,  0,0,0,9,0], CodeAdress(2), StackAddress(0), 20000000);
		core.set_register(Register::Address(0), 10);
		core.set_register(Register::Address(1), 26);
		//core.set_register(Register::Address(7), 0x12345678);
		core.set_register(Register::Data(0), 0x12348678);
		core.set_register(Register::Data(1), 0x8001);
		core.set_register(Register::Data(4), 0x700);
		core.set_register(Register::Data(5), 0x8001);
		
		// full prefetch
		for _ in 0..64 {
            core.step();
        }			

		let steps = ( 4 +12 + 4  + 3  ) * 8 + 0;
		for _ in 0 .. steps {
          core.step();
		}
		
		core.setIPL(7);
		let steps =  (16+ 12)  * 8 + 5;
	 	
		for _ in 0 .. steps {
          core.step();
		}
		
		let state = core.cpu_state();
		println!("ra {} rv {} wa {} wv {} ssp {} usp {} d0 {} a0 {} a7 {}  fc {} halt {} pc {} au {}  flags {}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[0], state.a_registers[0], state.a_registers[7], state.fc, state.halted, state.pc, state.au, state.flags);
		assert_eq!(101, 1);
	}

    #[test]
    fn test_divs() {
        let mut core = Fx68k::new_with_code(&[0x83, 0xc0, 0x83, 0xc0, 0x4e, 0x71, 0x4e, 0x71, 0x4e, 0x71, 0x33, 0x20], CodeAdress(0), StackAddress(0), 20000000);
        core.set_register(Register::Address(0), 10);
        //core.set_register(Register::Address(7), 1000);
        core.set_register(Register::Data(0), 4294967246);
        //core.set_register(Register::Data(0), 50);
        core.set_register(Register::Data(1), 100);

        // full prefetch
        for _ in 0..64 {
            core.step();
        }
//core.setIPL(7);

        let steps =  (72*2) * 8 - 8;

        for _ in 0 .. steps {
            core.step();
        }

        core.setIPL(7);
        let steps =  ( 1 + 4 + 2 +6 + 3)  * 8 + 5;

        for _ in 0 .. steps {
            core.step();
        }

        let state = core.cpu_state();

        println!("ra {} rv {} wa {} wv {} ssp {} usp {} d1 {} a0 {}  fc {} halt {} pc{}", state.last_read_address, state. read_val, state.last_written_address, state.written_val, state.ssp, state.usp, state.d_registers[1], state.a_registers[0], state.fc, state.halted, state.pc);
        assert_eq!(14, 1);
    }
}
