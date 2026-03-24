((nil . ((eval . (add-hook 'before-save-hook
                           (lambda ()
                             (when (and (bound-and-true-p lsp-mode)
                                        (fboundp 'lsp-format-buffer))
                               (lsp-format-buffer)))
                           nil t))
         (projectile-project-compilation-cmd . "scripts/build.py --config=debug --build --install")
         (eval . (progn
                   (setq +format-on-save-enabled-modes t))))))
