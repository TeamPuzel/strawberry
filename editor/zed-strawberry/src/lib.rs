use zed_extension_api::{self as zed, Result};

struct StrawberryExtension;

impl zed::Extension for StrawberryExtension {
    fn new() -> Self { Self }

    fn language_server_command(
        &mut self, _language_server_id: &zed::LanguageServerId, worktree: &zed::Worktree,
    ) -> Result<zed::Command> {
        let strc_path = worktree
            .which("strc")
            .ok_or_else(|| "strc executable not found in PATH. Make sure your C++ compiler is built and available.".to_string())?;

        Ok(zed::Command {
            command: strc_path,
            args: vec!["serve".to_string()],
            env: vec![],
        })
    }
}

zed::register_extension!(StrawberryExtension);
