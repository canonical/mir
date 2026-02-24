use proc_macro2::{Literal, TokenStream};
use quote::{format_ident, quote};
use syn::Ident;

pub struct CppBuilder {
    guard_name: String,
    pub filename: String,
    namespaces: Vec<CppNamespace>,
    includes: Vec<String>,
}

impl CppBuilder {
    pub fn new(guard_name: String, filename: String) -> CppBuilder {
        CppBuilder {
            guard_name,
            filename,
            namespaces: vec![],
            includes: vec![],
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

    /// Output the C++ content to a string for the header file.
    pub fn to_cpp_header(&self) -> String {
        let mut result = String::new();

        // Add include guard_name
        result.push_str(&format!("#ifndef {}\n", self.guard_name));
        result.push_str(&format!("#define {}\n\n", self.guard_name));

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
            for forward in &namespace.forward_declarations {
                result.push_str(&format!("{};\n", forward));
            }

            // Generate classes
            for class in &namespace.classes {
                result.push_str(&format!("class {}\n", class.name));
                result.push_str("{\n");
                result.push_str("public:\n");

                // Generate enums
                for enum_ in &class.enums {
                    result.push_str(&format!("    enum class {} : uint32_t\n", enum_.name));
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

                    let retstring = method
                        .retval
                        .as_ref()
                        .map(cpp_type_to_string)
                        .unwrap_or("void".to_string());

                    let method_name = sanitize_identifier(&method.name);
                    result.push_str(&format!(
                        "    virtual auto {}({}) -> {} = 0;\n",
                        method_name, args_str, retstring
                    ));
                }

                result.push_str("};\n\n");
            }

            // Close namespace(s)
            for _ in &namespace.name {
                result.push_str("}\n");
            }
        }

        // Close include guard_name
        result.push_str(&format!("\n#endif  // {}\n", self.guard_name));

        result
    }

    /// Generate Rust binding declarations for this C++ header.
    ///
    /// Returns a `Vec<TokenStream>` where each element contains the `include!` directive,
    /// type declaration, and methods for a single C++ class, intended to be placed inside
    /// an `unsafe extern "C++"` block. This method does NOT add the surrounding `mod ffi`
    /// or `unsafe extern "C++"` blocks; callers are responsible for adding those themselves.
    pub fn to_rust_bindings(&self) -> Vec<TokenStream> {
        let header_name = Literal::string(format!("include/{}.h", self.filename).as_str());
        // Include the corresponding C++ header once per protocol/header.
        let mut tokens: Vec<TokenStream> = Vec::new();
        tokens.push(quote! {
            include!(#header_name);
        });
        for namespace in &self.namespaces {
            let namespace_str = namespace.name.join("::");
            for class in &namespace.classes {
                let class_name = format_ident!("{}", class.name);
                // Generate methods for this class
                let methods = class.methods.iter().map(|method| {
                    let method_name = format_ident!("{}", sanitize_identifier(&method.name));
                    let args = method.args.iter().map(|arg| {
                        let arg_name = format_ident!("{}", sanitize_identifier(&arg.name));
                        let arg_type = cpp_type_to_rust_type(&arg.cpp_type, false);
                        quote! { #arg_name: #arg_type }
                    });

                    if let Some(retval) = &method.retval {
                        let retval = cpp_type_to_rust_type(retval, true);
                        quote! {
                            pub fn #method_name(self: &#class_name, #(#args),*) -> #retval;
                        }
                    } else {
                        quote! {
                            pub fn #method_name(self: &#class_name, #(#args),*);
                        }
                    }
                });
                tokens.push(quote! {
                    #[namespace = #namespace_str]
                    pub type #class_name;
                    #(#methods)*
                });
            }
        }
        tokens
    }
}

pub struct CppNamespace {
    pub name: Vec<String>,
    pub classes: Vec<CppClass>,
    forward_declarations: Vec<String>,
}

impl CppNamespace {
    pub fn new(name: Vec<String>) -> CppNamespace {
        CppNamespace {
            name: name,
            classes: vec![],
            forward_declarations: vec![],
        }
    }

    pub fn add_class(&mut self, class: CppClass) -> &mut CppClass {
        self.classes.push(class);
        self.classes
            .last_mut()
            .expect("classes cannot be empty after push")
    }

    pub fn add_forward_declaration_class(&mut self, name: &str) {
        self.forward_declarations.push(format!("class {}", name));
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
    pub retval: Option<CppType>,
}

impl CppMethod {
    pub fn new(name: String, retval: Option<CppType>) -> CppMethod {
        CppMethod {
            name,
            args: vec![],
            retval,
        }
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
    CppF64,
    String,
    Object(String),
    Array,
    Fd,
}

fn cpp_type_to_string(cpp_type: &CppType) -> String {
    match cpp_type {
        CppType::CppI32 => "int32_t".to_string(),
        CppType::CppU32 => "uint32_t".to_string(),
        CppType::CppF64 => "double".to_string(),
        CppType::String => "rust::String const&".to_string(),
        CppType::Object(name) => format!("std::unique_ptr<{}> const&", name),
        CppType::Array => "rust::Vec<uint8_t> const&".to_string(),
        CppType::Fd => "int32_t".to_string(),
    }
}

fn cpp_type_to_rust_type(cpp_type: &CppType, is_retval: bool) -> TokenStream {
    match cpp_type {
        CppType::CppI32 => quote! { i32 },
        CppType::CppU32 => quote! { u32 },
        CppType::CppF64 => quote! { f64 },
        CppType::String => quote! { String },
        CppType::Object(name) => {
            let type_name = format_ident!("{}", name);
            if is_retval {
                quote! { UniquePtr<#type_name> }
            } else {
                quote! { &UniquePtr<#type_name> }
            }
        }
        CppType::Array => quote! { Vec<u8> },
        CppType::Fd => quote! { i32 },
    }
}

/// Sanitize an identifier to ensure it's valid for Rust.
/// If the identifier starts with a digit, prefix it with an underscore.
/// If the identifier is a Rust keyword, prefix it with r#.
pub fn sanitize_identifier(name: &str) -> String {
    if name.is_empty() {
        return "_empty".to_string();
    }

    // Try to parse as a regular identifier
    // If it fails (e.g., it's a keyword), use raw identifier
    match syn::parse_str::<Ident>(name) {
        Ok(_) => name.to_string(),
        Err(_) => format!("r#{}", name),
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
