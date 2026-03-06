use proc_macro2::TokenStream;
use quote::format_ident;
use std::fs;
use syn::Ident;

/// Write the generated Rust code to a file with proper formatting.
pub fn write_generated_rust_file(tokens: TokenStream, filename: &str) {
    let out_dir = "src";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    let syntax_tree: syn::File = syn::parse2(tokens).unwrap();
    let formatted_code = prettyplease::unparse(&syntax_tree);

    fs::write(dest_path, formatted_code).unwrap();
}

/// Write the generated C++ code to the correct directory.
pub fn write_generated_cpp_file(content: &str, filename: &str) {
    let out_dir = "include";
    let dest_path = std::path::Path::new(&out_dir).join(filename);

    fs::create_dir_all(&out_dir).unwrap();
    fs::write(dest_path, content).unwrap();
}

pub fn dash_to_snake(name: &str) -> String {
    name.replace('-', "_")
}

pub fn dash_to_snake_ident(name: &str) -> Ident {
    format_ident!("{}", dash_to_snake(name))
}

pub fn snake_to_pascal(s: &str) -> String {
    s.split('_')
        .map(|word| {
            let mut chars = word.chars();
            match chars.next() {
                None => String::new(),
                Some(first) => first.to_uppercase().collect::<String>() + chars.as_str(),
            }
        })
        .collect()
}

pub fn format_cpp_class_name(s: &String) -> String {
    format!("{}Impl", snake_to_pascal(s))
}
