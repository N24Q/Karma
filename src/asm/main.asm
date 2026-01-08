;main.asm - Updated for Filament renderer with pixelation
BITS 64

extern InitialiseWindow
extern CleanExitWindow
extern glfwPollEvents
extern glfwWindowShouldClose
extern windowPtr
extern PlayVideo 
extern InitRenderer
extern LoadGLTF
extern SetModelTransform
extern RenderFrame
extern CleanupRenderer
extern Sleep
extern printf
extern PlayAudio
extern PlayAudioAsync
extern SetCameraTransform
extern glfwShowWindow
extern SetPixelationAmount
extern DisablePixelation

global program_startup

section .rodata
    videoPath db "assets/GrandGambit64IntroProtoWide.mp4", 0
    modelPath db "assets/models/basementroom9-cTextures/basementroom9-cTextures.gltf", 0
    audioPath db "assets/audio/Buckshot Roulette-Main Menu.mp3", 0

    msgVideoComplete db "[ASM] Video playback complete", 10, 0
    msgCheckWindow db "[ASM] Checking window after video...", 10, 0
    msgWindowOK db "[ASM] Window is OK, continuing...", 10, 0
    msgWindowClosed db "[ASM] ERROR: Window was closed!", 10, 0
    msgEnteringLoop db "[ASM] Entering render loop...", 10, 0
    msgExiting db "[ASM] Exiting render loop", 10, 0

    msgRendererInitFailed db "[ASM] ERROR: Renderer initialization failed!", 10, 0
    msgModelLoadFailed db "[ASM] ERROR: Model loading failed!", 10, 0

    msgRendererInitPassed db "[ASM] Filament renderer initialized successfully", 10, 0
    msgLoadingModel db "[ASM] Loading model...", 10, 0
    msgModelLoadPassed db "[ASM] Model loaded successfully", 10, 0
    msgPixelationEnabled db "[ASM] Pixelation effect enabled", 10, 0
    msgPixelationDisabled db "[ASM] Pixelation effect disabled", 10, 0
    
    ; Model position/scale/rotation values
    pos_x dd 0.0        ; right/left
    pos_y dd 0.0        ; up/down
    pos_z dd 0.0        ; depth - at origin
    scale_val dd 1.0    ; scale for all axes
    yaw_val   dd 0.0    ; yaw (radians)
    pitch_val dd 0.0    ; pitch (radians)

    ; Camera position and focus point
    ; Model center: (1.05, 3.51, 0.36), largest extent: ~15.35
    ; Position camera to view the model from a good angle
    camera_pos_x dd 0.0
    camera_pos_y dd 3.5
    camera_pos_z dd 12.5
    camera_focus_pos_x dd 0.0
    camera_focus_pos_y dd 3.5
    camera_focus_pos_z dd 0.0

    ; Pixelation scale values (0.01 to 1.0)
    ; Lower = more pixelated, Higher = less pixelated
    ; Values chosen to give integer pixel blocks at 1920x1080
    pixel_scale_ultra dd 0.02     ; 2% = 50 pixel blocks
    pixel_scale_extreme dd 0.05   ; 5% = 20 pixel blocks
    pixel_scale_very_low dd 0.1   ; 10% = 10 pixel blocks
    pixel_scale_low dd 0.125      ; 12.5% = 8 pixel blocks (cleaner)
    pixel_scale_med dd 0.2        ; 20% = 5 pixel blocks
    pixel_scale_high dd 0.4      ; 50% = 2 pixel blocks
    pixel_scale_off dd 1.0        ; 100% = no effect

    ; UI update delta time (60 FPS = ~0.016 seconds)
    deltaTime dq 0.016

section .bss
    modelCopy resb 512  ; Buffer to hold copied model path
    frameCounter resd 1 ; Frame counter for effects

section .text
program_startup:
    sub rsp, 200

    ; Initialize frame counter
    mov dword [rel frameCounter], 0

    ; Initialize window
    call InitialiseWindow
    test eax, eax
    jnz .init_failed

    ; Optional: Play intro video (commented out for now)
    ;lea rcx, [rel videoPath]
    ;mov rdx, [rel windowPtr]
    ;call PlayVideo
    
    ;lea rcx, [rel msgVideoComplete]
    ;call printf

    ; Make sure window is visible
    mov rcx, [rel windowPtr]
    call glfwShowWindow

    ; Process events
    call glfwPollEvents
    
    ; Check window status
    lea rcx, [rel msgCheckWindow]
    call printf
    
    mov rcx, [rel windowPtr]
    test rcx, rcx
    jz .window_closed
    
    call glfwWindowShouldClose
    test eax, eax
    jnz .window_closed
    
    lea rcx, [rel msgWindowOK]
    call printf

    ;Play ambient audio
    lea rcx, [rel audioPath]
    mov edx, 50             ; Volume 0-100
    call PlayAudioAsync

    ; Initialize Filament renderer
    call InitRenderer
    test eax, eax
    jnz .render_init_failed
    lea rcx, [rel msgRendererInitPassed]
    call printf

    ; Set camera position and target BEFORE loading model
    sub rsp, 64    ; Allocate shadow space + parameter space

    movss xmm0, [rel camera_pos_x]       ; camPosX
    movss xmm1, [rel camera_pos_y]       ; camPosY
    movss xmm2, [rel camera_pos_z]       ; camPosZ
    movss xmm3, [rel camera_focus_pos_x] ; focusX

    mov eax, [rel camera_focus_pos_y]
    mov [rsp + 32], eax                   ; focusY

    mov eax, [rel camera_focus_pos_z]
    mov [rsp + 40], eax                   ; focusZ

    call SetCameraTransform

    ; Enable pixelation effect
    movss xmm0, [rel pixel_scale_high]    ; 15% resolution = 6-7 pixel blocks
    call SetPixelationAmount
    add rsp, 64

    ; Copy model path to writable buffer
    lea rsi, [rel modelPath]
    lea rdi, [rel modelCopy]
    xor rcx, rcx
.copy_loop:
    lodsb
    stosb
    test al, al
    jz .copy_done
    inc rcx
    cmp rcx, 511
    jb .copy_loop
.copy_done:

    ; Load GLTF model (now with responsive window due to event polling)
    lea rcx, [rel msgLoadingModel]
    call printf

    lea rcx, [rel modelCopy]
    call LoadGLTF
    test eax, eax
    jz .model_load_failed
    lea rcx, [rel msgModelLoadPassed]
    call printf

    ; Set model transform (position, scale, rotation)
    sub rsp, 64    ; Allocate shadow space + parameter space

    ; First 4 float parameters in XMM registers
    movss xmm0, [rel pos_x]       ; posX = 0.0
    movss xmm1, [rel pos_y]       ; posY = 0.0
    movss xmm2, [rel pos_z]       ; posZ = 0.0
    movss xmm3, [rel scale_val]   ; scaleX = 1.0

    ; Remaining parameters on stack (8-byte aligned)
    mov eax, [rel scale_val]
    mov [rsp + 32], eax            ; scaleY at +32

    mov eax, [rel scale_val]
    mov [rsp + 40], eax            ; scaleZ at +40

    mov eax, [rel yaw_val]
    mov [rsp + 48], eax            ; yaw at +48

    mov eax, [rel pitch_val]
    mov [rsp + 56], eax            ; pitch at +56

    call SetModelTransform
    add rsp, 64

    ; Enter main render loop
    lea rcx, [rel msgEnteringLoop]
    call printf

.render_loop:
    ; Increment frame counter
    inc dword [rel frameCounter]

    ; Optional: Change pixelation dynamically
    ; Uncomment the following to cycle through different pixelation levels
    ; mov eax, [rel frameCounter]
    ; and eax, 0x1FF              ; Every 512 frames (~8.5 seconds at 60fps)
    ; jnz .skip_pixelation_change
    ; 
    ; ; Cycle between different pixelation levels
    ; mov eax, [rel frameCounter]
    ; shr eax, 9                  ; Divide by 512
    ; and eax, 3                  ; Modulo 4 (0, 1, 2, 3)
    ; 
    ; cmp eax, 0
    ; je .set_very_pixelated
    ; cmp eax, 1
    ; je .set_med_pixelated
    ; cmp eax, 2
    ; je .set_light_pixelated
    ; jmp .set_no_pixelation
    ; 
    ; .set_very_pixelated:
    ;     movss xmm0, [rel pixel_scale_very_low]
    ;     call SetPixelationAmount
    ;     jmp .skip_pixelation_change
    ; 
    ; .set_med_pixelated:
    ;     movss xmm0, [rel pixel_scale_med]
    ;     call SetPixelationAmount
    ;     jmp .skip_pixelation_change
    ; 
    ; .set_light_pixelated:
    ;     movss xmm0, [rel pixel_scale_high]
    ;     call SetPixelationAmount
    ;     jmp .skip_pixelation_change
    ; 
    ; .set_no_pixelation:
    ;     call DisablePixelation
    ; 
    ; .skip_pixelation_change:

    ; Render 3D scene
    call RenderFrame

    ; Process window events
    call glfwPollEvents
    
    ; Frame rate control (~60 FPS)
    mov rcx, 8
    call Sleep

    ; Check if window should close
    mov rcx, [rel windowPtr]
    test rcx, rcx
    jz .exit_render_loop
    
    call glfwWindowShouldClose
    test eax, eax
    jz .render_loop

.exit_render_loop:
    lea rcx, [rel msgExiting]
    call printf

    ; Disable pixelation before cleanup
    call DisablePixelation
    lea rcx, [rel msgPixelationDisabled]
    call printf

    ; Cleanup Filament resources
    call CleanupRenderer

    ; Close window
    call CleanExitWindow

    xor eax, eax        ; Success
    add rsp, 200
    ret

.window_closed:
    lea rcx, [rel msgWindowClosed]
    call printf
    call CleanExitWindow
    mov eax, -4
    add rsp, 200
    ret

.model_load_failed:
    lea rcx, [rel msgModelLoadFailed]
    call printf
    call CleanupRenderer
    call CleanExitWindow
    mov eax, -3
    add rsp, 200
    ret

.render_init_failed:
    lea rcx, [rel msgRendererInitFailed]
    call printf
    call CleanExitWindow
    mov eax, -2
    add rsp, 200
    ret

.init_failed:
    mov eax, -1
    add rsp, 200
    ret