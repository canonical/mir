pub struct CppBuilder {
    guards: String,
    namespaces: Vec<CppNamespace>,
    includes: Vec<String>,
    forward_declarations: Vec<String>,
}

impl CppBuilder {
    pub fn new(guards: String) -> CppBuilder {
        CppBuilder {
            guards,
            namespaces: vec![],
            includes: vec![],
            forward_declarations: vec![],
        }
    }

    /// Add a new namespace to the builder.
    ///
    /// Returns the added namespace.
    pub fn add_namespace(&mut self, namespace: CppNamespace) -> &mut CppNamespace {
        self.namespaces.push(namespace);
        self.namespaces
            .last_mut()
            .expect("namespaces cannot be empty after push")
    }

    pub fn add_include(&mut self, include: String) {
        self.includes.push(include);
    }

    pub fn add_forward_declaration_class(&mut self, name: &str) {
        self.forward_declarations.push(format!("class {}", name));
    }

    pub fn to_string(&self) -> String {
        let mut result = String::new();

        // Add include guards
        result.push_str(&format!("#ifndef {}\n", self.guards));
        result.push_str(&format!("#define {}\n\n", self.guards));

        // Add includes
        for include in &self.includes {
            result.push_str(&format!("#include {}\n", include));
        }

        for namespace in &self.namespaces {
            // Open namespace(s)
            for name in &namespace.name {
                result.push_str(&format!("namespace {}\n{{\n", name));
            }

            // Add forward declarations.
            for forward in &self.forward_declarations {
                result.push_str(&format!("{};\n", forward));
            }

            // Generate classes
            for class in &namespace.classes {
                result.push_str(&format!("class {}\n", class.name));
                result.push_str("{\n");
                result.push_str("public:\n");

                // Generate enums
                for enum_ in &class.enums {
                    result.push_str(&format!(
                        "    enum class {} : public uint32_t\n",
                        enum_.name
                    ));
                    result.push_str("    {\n");

                    for option in &enum_.options {
                        result.push_str(&format!(
                            "        {} = {},\n",
                            option.name,
                            option.value.to_string(),
                        ));
                    }
                    result.push_str("    };\n");
                    result.push_str("\n");
                }

                // Virtual destructor
                result.push_str(&format!("    virtual ~{}() = default;\n\n", class.name));

                // Generate methods
                for method in &class.methods {
                    let args: Vec<String> = method
                        .args
                        .iter()
                        .map(|arg| format!("{} {}", cpp_type_to_string(&arg.cpp_type), arg.name))
                        .collect();
                    let args_str = args.join(", ");

                    result.push_str(&format!(
                        "    virtual auto {}({}) = 0;\n",
                        method.name, args_str
                    ));
                }

                result.push_str("};\n\n");
            }

            // Close namespace(s)
            for _ in &namespace.name {
                result.push_str("}\n");
            }
        }

        // Close include guards
        result.push_str(&format!("\n#endif  // {}\n", self.guards));

        result
    }
}

pub struct CppNamespace {
    pub name: Vec<String>,
    pub classes: Vec<CppClass>,
}

impl CppNamespace {
    pub fn new(name: Vec<String>) -> CppNamespace {
        CppNamespace {
            name: name,
            classes: vec![],
        }
    }

    pub fn add_class(&mut self, class: CppClass) -> &mut CppClass {
        self.classes.push(class);
        self.classes
            .last_mut()
            .expect("classes cannot be empty after push")
    }
}

pub struct CppClass {
    pub name: String,
    pub methods: Vec<CppMethod>,
    pub enums: Vec<CppEnum>,
}

impl CppClass {
    pub fn new(name: String) -> CppClass {
        CppClass {
            name,
            methods: vec![],
            enums: vec![],
        }
    }

    pub fn add_method(&mut self, method: CppMethod) -> &mut CppMethod {
        self.methods.push(method);
        self.methods
            .last_mut()
            .expect("methods cannot be empty after push")
    }

    pub fn add_enum(&mut self, cpp_enum: CppEnum) -> &mut CppEnum {
        self.enums.push(cpp_enum);
        self.enums
            .last_mut()
            .expect("enums cannot be empty after push")
    }
}

pub struct CppEnum {
    pub name: String,
    pub options: Vec<CppEnumOption>,
}

impl CppEnum {
    pub fn new(name: String) -> CppEnum {
        CppEnum {
            name,
            options: vec![],
        }
    }

    pub fn add_option(&mut self, option: CppEnumOption) -> &mut CppEnumOption {
        self.options.push(option);
        self.options
            .last_mut()
            .expect("enums cannot be empty after push")
    }
}

pub struct CppEnumOption {
    pub name: String,
    pub value: u32,
}

pub struct CppMethod {
    pub name: String,
    pub args: Vec<CppArg>,
}

impl CppMethod {
    pub fn new(name: String) -> CppMethod {
        CppMethod { name, args: vec![] }
    }

    pub fn add_arg(&mut self, arg: CppArg) -> &mut CppArg {
        self.args.push(arg);
        self.args
            .last_mut()
            .expect("args cannot be empty after push")
    }
}

pub enum CppType {
    CppI32,
    CppU32,
    CppF32,
    String,
    Object(String),
    NewId(String),
    Array,
    Fd,
}

fn cpp_type_to_string(cpp_type: &CppType) -> String {
    match cpp_type {
        CppType::CppI32 => "int32_t".to_string(),
        CppType::CppU32 => "uint32_t".to_string(),
        CppType::CppF32 => "float".to_string(),
        CppType::String => "rust::Str const&".to_string(),
        CppType::Object(name) => format!("rust::SharedPtr<{}> const&", name),
        CppType::NewId(interface) => "int32_t".to_string(), // TODO
        CppType::Array => "rust::Vec<uint8_t> const&".to_string(),
        CppType::Fd => "int32_t".to_string(),
    }
}

pub struct CppArg {
    pub cpp_type: CppType,
    pub name: String,
}

impl CppArg {
    pub fn new(cpp_type: CppType, name: String) -> CppArg {
        CppArg { cpp_type, name }
    }
}
