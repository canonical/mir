use proc_macro2::{Literal, TokenStream};
use quote::{format_ident, quote};
use syn::Ident;

use crate::helpers::format_has_arg;

pub struct CppBuilder {
    guard_name: String,
    pub filename: String,
    namespaces: Vec<CppNamespace>,

    /// Includes in the generated .h file.
    header_includes: Vec<String>,

    /// Includes in the generated .cpp file.
    cpp_includes: Vec<String>,
}

impl CppBuilder {
    pub fn new(guard_name: impl Into<String>, filename: impl Into<String>) -> CppBuilder {
        CppBuilder {
            guard_name: guard_name.into(),
            filename: filename.into(),
            namespaces: vec![],
            header_includes: vec![],
            cpp_includes: vec![],
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

    pub fn add_header_include(&mut self, include: impl Into<String>) {
        self.header_includes.push(include.into());
    }

    pub fn add_cpp_include(&mut self, include: impl Into<String>) {
        self.cpp_includes.push(include.into());
    }

    fn build_ret_string_for_cpp(method: &CppMethod) -> String {
        method
            .retval
            .as_ref()
            .map(cpp_return_type_to_cpp_source)
            .unwrap_or("void".to_string())
    }

    fn build_arg_str_for_cpp(method: &CppMethod) -> String {
        let cpp_method_num_args = method
            .args
            .iter()
            .map(|arg| if arg.has_name.is_some() { 2 } else { 1 })
            .sum();
        let mut args: Vec<String> = Vec::with_capacity(cpp_method_num_args);
        for arg in &method.args {
            args.push(format!(
                "{} {}",
                cpp_arg_type_to_cpp_source(&arg.cpp_type, method.originates_from_rust),
                arg.name
            ));

            if let Some(has_name) = &arg.has_name {
                args.push(format!("bool {}", has_name));
            }
        }
        args.join(", ")
    }

    /// Generates the .h file contents corresponding to the information in this builder.
    pub fn to_cpp_header(&self) -> String {
        let mut result = String::new();

        // Add include guard_name
        result.push_str(&format!("#ifndef {}\n", self.guard_name));
        result.push_str(&format!("#define {}\n\n", self.guard_name));

        // Add includes
        for include in &self.header_includes {
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
                if let Some(superclass) = &class.superclass {
                    result.push_str(&format!("class {} : public {}\n", class.name, superclass));
                } else {
                    result.push_str(&format!("class {}\n", class.name));
                }
                result.push_str("{\n");
                result.push_str("public:\n");

                // Generate enums
                //
                // Enums are generated as structs so that they are easily usable as uint32_t in
                // places that the protocol expects. This is how the old generator managed this.
                for enum_ in &class.enums {
                    result.push_str(&format!("    struct {}\n", enum_.name));
                    result.push_str("    {\n");

                    for option in &enum_.options {
                        result.push_str(&format!(
                            "        static constexpr uint32_t {} = {};\n",
                            option.name, option.value,
                        ));
                    }
                    result.push_str("    };\n");
                    result.push_str("\n");
                }

                // Virtual destructor
                result.push_str(&format!("    virtual ~{}() = default;\n\n", class.name));

                // Generate methods
                for method in &class.methods {
                    let args_str = Self::build_arg_str_for_cpp(method);
                    let retstring = Self::build_ret_string_for_cpp(method);
                    if method.is_virtual {
                        if method.body.is_some() {
                            result.push_str(&format!(
                                "    virtual auto {}({}) -> {};\n",
                                method.name, args_str, retstring
                            ));
                        } else {
                            result.push_str(&format!(
                                "    virtual auto {}({}) -> {} = 0;\n",
                                method.name, args_str, retstring
                            ));
                        }
                    } else {
                        result.push_str(&format!(
                            "    auto {}({}) -> {};\n",
                            method.name, args_str, retstring
                        ));
                    }
                }

                for member in &class.public_members {
                    let const_str = if member.is_const { " const" } else { "" };
                    result.push_str(&format!(
                        "    {}{} {};\n",
                        cpp_arg_type_to_cpp_source(&member.cpp_type, true),
                        const_str,
                        member.name
                    ));
                }

                if !class.protected_constructor_args.is_empty()
                    || !class.protected_members.is_empty()
                {
                    result.push_str("protected:\n");

                    if !class.protected_constructor_args.is_empty() {
                        let ctor_args_str: Vec<String> = class
                            .protected_constructor_args
                            .iter()
                            .map(|arg| {
                                format!(
                                    "{} {}",
                                    cpp_arg_type_to_cpp_source(&arg.cpp_type, true),
                                    arg.name
                                )
                            })
                            .collect();
                        let initialiser_list: Vec<String> = class
                            .protected_constructor_args
                            .iter()
                            .map(|arg| format!("{} ( std::move({}) )", arg.name, arg.name))
                            .collect();
                        result.push_str(&format!(
                            "    {}({}) : {} {{}}\n",
                            class.name,
                            ctor_args_str.join(", "),
                            initialiser_list.join(", ")
                        ));
                        result.push_str("\n");
                    }

                    for member in &class.protected_members {
                        let const_str = if member.is_const { " const" } else { "" };
                        result.push_str(&format!(
                            "    {}{} {};\n",
                            cpp_arg_type_to_cpp_source(&member.cpp_type, true),
                            const_str,
                            member.name
                        ));
                    }

                    result.push_str("\n");
                }

                result.push_str("private:\n");
                for member in &class.private_members {
                    let const_str = if member.is_const { " const" } else { "" };
                    if member.optional {
                        result.push_str(&format!(
                            "    std::optional<{}>{} {};\n",
                            cpp_arg_type_to_cpp_source(&member.cpp_type, true),
                            const_str,
                            member.name
                        ));
                    } else {
                        result.push_str(&format!(
                            "    {}{} {};\n",
                            cpp_arg_type_to_cpp_source(&member.cpp_type, true),
                            const_str,
                            member.name
                        ));
                    }
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

    /// Generates the .cpp file contents corresponding to the information in this builder.
    pub fn to_cpp_source(&self, header_path: &str) -> String {
        let mut result = String::new();
        result.push_str(&format!("#include \"{}\"\n", header_path));

        for include in &self.cpp_includes {
            result.push_str(&format!("#include {}\n", include));
        }

        for namespace in &self.namespaces {
            let namespace_str = namespace.name.join("::");
            for class in &namespace.classes {
                for method in &class.methods {
                    if method.body.is_none() {
                        continue;
                    }

                    let args_str = Self::build_arg_str_for_cpp(method);
                    let retstring = Self::build_ret_string_for_cpp(method);

                    result.push_str(&format!(
                        "auto {}::{}::{}({}) -> {}\n",
                        namespace_str, class.name, method.name, args_str, retstring
                    ));
                    result.push_str("{\n");
                    let body = method
                        .body
                        .as_deref()
                        .unwrap_or("// TODO: Call out to Rust code here.");
                    result.push_str(&format!("    {}\n", body));
                    result.push_str("}\n\n");
                }
            }
        }

        result
    }

    /// Generate Rust binding declarations for this C++ header.
    ///
    /// The Rust side will use these bindings to call into C++.
    ///
    /// Returns a `Vec<TokenStream>` where each element contains the `include!` directive,
    /// type declaration, and methods for a single C++ class, intended to be placed inside
    /// an `unsafe extern "C++"` block. This method does NOT add the surrounding `mod ffi`
    /// or `unsafe extern "C++"` blocks; callers are responsible for adding those themselves.
    pub fn to_rust_cpp_bindings(&self) -> Vec<TokenStream> {
        let header_name =
            Literal::string(format!("wayland_rs_cpp/include/{}.h", self.filename).as_str());
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
                let methods = class.methods.iter().flat_map(|method| {
                    if !method.generates_rust_code {
                        return None;
                    }

                    let method_name = format_ident!("{}", method.name);
                    let args = method.args.iter().flat_map(|arg| {
                        let arg_name = format_ident!("{}", arg.name);

                        // If the argument is optional, we provide a boolean describing if it is set
                        // or not, since `std::optional` cannot be ferried over the C++/Rust boundary.
                        let arg_type = cpp_arg_type_to_rust_source(
                            &arg.cpp_type,
                            method.originates_from_rust,
                        );
                        let main_arg = quote! { #arg_name: #arg_type };
                        if let Some(has_name) = &arg.has_name {
                            let has_arg_name = format_ident!("{}", has_name);
                            vec![main_arg, quote! { #has_arg_name: bool }]
                        } else {
                            vec![main_arg]
                        }
                    });

                    // Note: When generating Rust bindings for C++ methods that will mutate the underlying
                    // C++ class, cxx.rs enforces that we `Pin` them.
                    //
                    // Note: If the method throws, we wrap it in a Result<...> type for the Rust side of
                    // things. cxx.rs will take care of translating exceptions into this type for us.
                    if let Some(retval) = &method.retval {
                        let retval = cpp_return_type_to_rust_source(retval);
                        let retval = if method.throws { quote!{ Result<#retval> } } else { retval };
                        Some(quote! {
                            pub fn #method_name(self: Pin<&mut #class_name>, #(#args),*) -> #retval;
                        })
                    } else {
                        if method.throws {
                            Some(quote! {
                                pub fn #method_name(self: Pin<&mut #class_name>, #(#args),*) -> Result<()>;
                            })
                        }
                        else {
                            Some(quote! {
                                pub fn #method_name(self: Pin<&mut #class_name>, #(#args),*);
                            })
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
    pub fn new<I, S>(name: I) -> CppNamespace
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        CppNamespace {
            name: name.into_iter().map(Into::into).collect(),
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

    pub fn add_forward_declaration_struct(&mut self, name: &str) {
        self.forward_declarations.push(format!("struct {}", name));
    }

    pub fn add_forward_declaration_class(&mut self, name: &str) {
        self.forward_declarations.push(format!("class {}", name));
    }
}

pub struct CppClass {
    pub name: String,
    pub superclass: Option<String>,
    pub methods: Vec<CppMethod>,
    pub enums: Vec<CppEnum>,
    pub public_members: Vec<CppArg>,
    pub protected_constructor_args: Vec<CppArg>,
    pub protected_members: Vec<CppArg>,
    pub private_members: Vec<CppArg>,
}

impl CppClass {
    pub fn new(name: impl Into<String>) -> CppClass {
        CppClass {
            name: sanitize_identifier(&name.into()),
            superclass: None,
            methods: vec![],
            enums: vec![],
            public_members: vec![],
            protected_constructor_args: vec![],
            protected_members: vec![],
            private_members: vec![],
        }
    }

    pub fn set_superclass(&mut self, name: impl Into<String>) {
        self.superclass = Some(name.into());
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

    pub fn add_public_member(&mut self, member: CppArg) -> &mut CppArg {
        self.public_members.push(member);
        self.public_members
            .last_mut()
            .expect("public_members cannot be empty after push")
    }

    pub fn add_protected_constructor_arg(&mut self, arg: CppArg) -> &mut CppArg {
        self.protected_constructor_args.push(arg);
        self.protected_constructor_args
            .last_mut()
            .expect("protected_constructor_args cannot be empty after push")
    }

    pub fn add_private_member(&mut self, member: CppArg) -> &mut CppArg {
        self.private_members.push(member);
        self.private_members
            .last_mut()
            .expect("members cannot be empty after push")
    }
}

pub struct CppEnum {
    pub name: String,
    pub options: Vec<CppEnumOption>,
}

impl CppEnum {
    pub fn new(name: impl Into<String>) -> CppEnum {
        CppEnum {
            name: sanitize_identifier(&name.into()),
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

impl CppEnumOption {
    pub fn new(name: impl Into<String>, value: u32) -> CppEnumOption {
        CppEnumOption {
            name: sanitize_identifier(&name.into()),
            value: value,
        }
    }
}

pub struct CppMethod {
    name: String,
    args: Vec<CppArg>,
    retval: Option<CppType>,
    is_virtual: bool,
    body: Option<String>,
    throws: bool,
    originates_from_rust: bool,
    generates_rust_code: bool,
}

impl CppMethod {
    pub fn new(
        name: impl Into<String>,
        retval: Option<CppType>,
        is_virtual: bool,
        throws: bool,
        originates_from_rust: bool,
        generates_rust_code: bool,
    ) -> CppMethod {
        CppMethod {
            name: sanitize_identifier(&name.into()),
            args: vec![],
            retval,
            is_virtual,
            body: None,
            throws,
            originates_from_rust,
            generates_rust_code,
        }
    }

    pub fn add_arg(&mut self, arg: CppArg) -> &mut CppArg {
        self.args.push(arg);
        self.args
            .last_mut()
            .expect("args cannot be empty after push")
    }

    // Set the body of the method.
    // This may be a more complicated "builder" some day, but we are doing such
    // simple stuff for now that we might as well make it a string
    pub fn set_body(&mut self, body: impl Into<String>) {
        self.body = Some(body.into());
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
    Box(String),
    Bool,
}

/// Convert a CppType intended as a return value to its corresponding
/// C++ source code string in the function signature.
fn cpp_return_type_to_cpp_source(cpp_type: &CppType) -> String {
    match cpp_type {
        CppType::CppI32 => "int32_t".to_string(),
        CppType::CppU32 => "uint32_t".to_string(),
        CppType::CppF64 => "double".to_string(),
        CppType::String => "std::string".to_string(),
        CppType::Object(name) => {
            format!("std::shared_ptr<{}>", name)
        }
        CppType::Array => "std::vector<uint8_t>".to_string(),
        CppType::Fd => "int32_t".to_string(),
        CppType::Box(name) => {
            format!("rust::Box<{}> const&", name)
        }
        CppType::Bool => "bool".to_string(),
    }
}

/// Convert a CppType for an argument to its corresponding C++ source
/// code string.
///
/// If the method is called from Rust, we will use the cxx.rs C++ wrapper
/// for the type instead of the standard library type.
fn cpp_arg_type_to_cpp_source(cpp_type: &CppType, originates_from_rust: bool) -> String {
    match (cpp_type, originates_from_rust) {
        (CppType::CppI32, _) => "int32_t".into(),
        (CppType::CppU32, _) => "uint32_t".into(),
        (CppType::CppF64, _) => "double".into(),
        (CppType::Fd, _) => "int32_t".into(),
        (CppType::Object(name), _) => format!("std::shared_ptr<{}> const&", name),
        (CppType::Box(name), _) => {
            if originates_from_rust {
                format!("rust::Box<{}>", name)
            } else {
                format!("rust::Box<{}> const&", name)
            }
        }
        (CppType::String, true) => "rust::String".into(),
        (CppType::String, false) => "std::string const&".into(),
        (CppType::Array, true) => "rust::Vec<uint8_t>".into(),
        (CppType::Array, false) => "std::vector<uint8_t> const&".into(),
        (CppType::Bool, _) => "bool".to_string(),
    }
}

/// Convert a CppType intended as a return value to its corresponding
/// Rust source code token in the function signature.
fn cpp_return_type_to_rust_source(cpp_type: &CppType) -> TokenStream {
    match cpp_type {
        CppType::CppI32 => quote! { i32 },
        CppType::CppU32 => quote! { u32 },
        CppType::CppF64 => quote! { f64 },
        CppType::String => quote! { &CxxString },
        CppType::Object(name) => {
            let type_name = format_ident!("{}", name);
            quote! { SharedPtr<#type_name> }
        }
        CppType::Array => quote! { &CxxVector<u8> },
        CppType::Fd => quote! { i32 },
        CppType::Box(name) => {
            let type_name = format_ident!("{}", name);
            quote! { &Box<#type_name> }
        }
        CppType::Bool => quote! { bool },
    }
}

/// Convert a CppType for an argument to its corresponding Rust source
/// code block.
///
/// If the method is called from Rust, we will use the cxx.rs C++ wrapper
/// for the type instead of the standard library type.
fn cpp_arg_type_to_rust_source(cpp_type: &CppType, originates_from_rust: bool) -> TokenStream {
    match cpp_type {
        CppType::CppI32 => quote! { i32 },
        CppType::CppU32 => quote! { u32 },
        CppType::CppF64 => quote! { f64 },
        CppType::String => {
            if originates_from_rust {
                quote! { String }
            } else {
                quote! { &CxxString }
            }
        }
        CppType::Object(name) => {
            let type_name = format_ident!("{}", name);
            quote! { &SharedPtr<#type_name> }
        }
        CppType::Array => {
            if originates_from_rust {
                quote! { Vec<u8> }
            } else {
                quote! { &CxxVector<u8> }
            }
        }
        CppType::Fd => quote! { i32 },
        CppType::Box(name) => {
            let type_name = format_ident!("{}", name);
            if originates_from_rust {
                quote! { Box<#type_name> }
            } else {
                quote! { &Box<#type_name> }
            }
        }
        CppType::Bool => quote! { bool },
    }
}

/// Sanitize an identifier to ensure it's valid for Rust and C++.
/// If the identifier starts with a digit, prefix it with r_.
/// If the identifier is a Rust keyword, prefix it with r_.
/// If the identifier is a C++ keyword, suffix it with _.
pub fn sanitize_identifier(name: &str) -> String {
    if name.is_empty() {
        return "_empty".to_string();
    }

    // These keywords originate from: https://en.cppreference.com/w/cpp/keywords.html
    const CPP_KEYWORDS: &[&str] = &[
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "concept",
        "const_cast",
        "consteval",
        "constexpr",
        "constinit",
        "co_await",
        "co_return",
        "co_yield",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "register",
        "reinterpret_cast",
        "requires",
        "short",
        "signed",
        "sizeof",
        "static_assert",
        "static_cast",
        "switch",
        "template",
        "this",
        "thread_local",
        "throw",
        "try",
        "typedef",
        "typeid",
        "typename",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
    ];

    // Try to parse as a regular identifier
    // If it fails (e.g., it's a Rust keyword), use raw identifier
    match syn::parse_str::<Ident>(name) {
        Ok(_) => {
            if CPP_KEYWORDS.contains(&name) {
                format!("{}_", name)
            } else {
                name.to_string()
            }
        }
        Err(_) => format!("r_{}", name),
    }
}

pub struct CppArg {
    cpp_type: CppType,
    name: String,
    has_name: Option<String>,
    optional: bool,
    is_const: bool,
}

impl CppArg {
    pub fn new(cpp_type: CppType, name: impl Into<String>, optional: bool) -> CppArg {
        let name = name.into();
        CppArg {
            cpp_type,
            name: sanitize_identifier(name.as_str()),
            has_name: if optional {
                Some(sanitize_identifier(&format_has_arg(&name.clone())))
            } else {
                None
            },
            optional,
            is_const: false,
        }
    }

    pub fn set_const(&mut self) -> &mut Self {
        self.is_const = true;
        self
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn has_name(&self) -> Option<String> {
        self.has_name.clone()
    }

    pub fn cpp_type(&self) -> &CppType {
        &self.cpp_type
    }
}
