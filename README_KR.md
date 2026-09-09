# zet

> C++20 표준 컨테이너 및 메모리 라이브러리 (ZET - Zero-allocated Execution Toolkit)
> 
> ZET는 내부에서 힙 메모리를 할당하지 않는 C++20 컨테이너 및 메모리 도구 모음입니다. 모든 용량은 컴파일 타임에 결정되거나 호출자가 제공한 외부 버퍼로 결정됩니다. ZET는 숨겨진 확장이나 힙 fallback을 수행하지 않습니다.

---

## 주요 특징

* **라이브러리 자체 무할당**: ZET 구현은 `new`, `delete`, `malloc`, `free`로 저장 공간을 확보하지 않습니다.
* **명시적 용량**: 고정 용량 컨테이너 또는 호출자가 제공한 메모리만 사용하며 자동 확장하지 않습니다.
* **예측 가능한 실패**: `Pool::Create`, `Pool::TryGet`, `CommandBuffer::Push` 등은 용량 초과나 잘못된 입력을 실패값으로 보고합니다.
* **C++20 최신 명세 적용**: concepts 및 requires 제약 조건을 통한 컴파일 타임 타입 검사 적용.
* **초경량 설계**: 불필요한 외적 종속성을 전면 배제하고 메모리 효율을 극대화.
* **xmake 시스템 연동**: 모던 빌드 툴 xmake를 활용한 빌드, 패키징 및 유닛 테스트의 완벽한 자동화.
* **크로스 플랫폼 검증**: Windows (MSVC), Linux (GCC), macOS (Clang) 3대 플랫폼 완벽 지원.

---

## 제공 컴포넌트 목록

### 1. 컨테이너 (Containers)
* **List.hpp**: 배열 기반 동적 리스트 (메모리 정적 할당 및 소멸자 제약 적용)
* **String.hpp**: 고정 크기 버퍼 최적화 문자열 클래스 (암시적 string_view 변환 및 결합 연산 지원)
* **Map.hpp**: 해시 함수를 활용한 고속 키-값 해시 맵 (중복 키 제어 및 이터레이션 지원)
* **Pool.hpp**: 고성능 리소스 풀 (세대(Generation) 기반 Dangling 핸들 추적 보호)
* **SparseSet.hpp**: 엔티티 관리에 최적화된 완전 고정 용량 스파스 셋. dense/sparse 저장 공간을 객체 내부에 보유하며 실행 중 페이지를 할당하지 않습니다.
* **CommandBuffer.hpp**: 비소유형 지연 명령 버퍼 (대량의 일괄 실행 명령어 배치 처리에 최적화)

### 2. 메모리 할당자 (Memory Allocators)
* **LinearAllocator.hpp**: 호출자가 제공한 버퍼 위에서 동작하며 Reset으로 전체 공간을 재사용합니다.
* **StackAllocator.hpp**: 호출자가 제공한 버퍼 위에서 LIFO 방식으로 동작하고 marker까지 되돌릴 수 있습니다.
* **PointerHandle.hpp**: allocator 내부 offset과 epoch를 저장합니다. Reset 또는 stack rewind 뒤의 오래된 핸들은 `Get()`에서 `nullptr`을 반환합니다.

### 외부 메모리 사용 예시

```cpp
#include <array>
#include <memory/LinearAllocator.hpp>

alignas(64) std::array<std::byte, 1024 * 1024> memory{};
zet::memory::LinearAllocator arena(memory);
auto value = arena.CreateHandle<int>(42);
```

ZET 컨테이너에 저장한 사용자 타입이나 콜백이 자체적으로 수행하는 할당은 ZET의 보장 범위에 포함되지 않습니다. 예를 들어 `List<std::string, 16>`의 저장 공간은 고정이지만 `std::string` 자체는 힙을 사용할 수 있습니다.

---

## 1. 로컬 빌드 및 유닛 테스트 방법

본 프로젝트는 doctest 기반 동작 테스트와 핵심 경로의 힙 할당 감시 테스트를 포함합니다.

### 빌드 및 실행 명령어

```bash
# 1. 디버그 모드로 프로젝트 구성 설정 (doctest 패키지 자동 설치)
xmake config --mode=debug --yes

# 2. 정적 라이브러리 및 테스트 파일 컴파일
xmake

# 3. 단언문(Assertion) 검증 테스트 실행
xmake test

# 4. 릴리즈 모드로 최적화 빌드 설정 전환
xmake config --mode=release
xmake
```

### 에디터(Sublime Text, VS Code 등) 완벽 연동 설정
에디터에서 C++20 표준(requires 등)을 실시간으로 분석해 컴파일 경고나 빨간 에러 표시를 방지할 수 있도록 에디터 자동완성 데이터베이스를 빌드 체인에 추가해 두었습니다. 빌드(xmake) 시마다 루트 폴더에 자동으로 업데이트됩니다.

* 수동 생성 필요 시: `xmake project -k compile_commands` 실행

---

## 2. 다른 Xmake 프로젝트에서 사용하는 방법

빌드된 zet 라이브러리를 다른 프로젝트에서 가져와 연동하는 가장 현대적이고 세련된 두 가지 방법을 제안합니다.

### 방식 [A] 하위 모듈 의존성 추가 (가장 추천)
소비자(다른 프로젝트)의 저장소에 zet 폴더를 하위 폴더나 Git 서브모듈(Submodule)로 추가한 뒤, 소비자 프로젝트의 xmake.lua 파일에 아래 내용을 기재합니다.

```lua
-- 1. zet 프로젝트 설정을 하위 프로젝트로 선언합니다.
includes("path/to/zet")

target("my_application")
    set_kind("binary")
    add_files("src/*.cpp")
    
    -- 2. 단 한 줄로 의존성을 연동합니다.
    -- (zet의 헤더 파일 검색 경로와 라이브러리 링크가 자동으로 전파됩니다!)
    add_deps("zet")
```

---

### 방식 [B] 깃허브 저장소를 원격 패키지로 직접 인스톨 (GitHub Package)
개발 중인 로컬 폴더 경로에 구애받지 않고, 인터넷을 통해 깃허브에서 직접 라이브러리를 종속성 패키지로 인스톨하여 사용하는 방식입니다. 마치 npm이나 pip처럼 작동합니다.

소비자 프로젝트의 xmake.lua 파일 상단에 다음과 같이 패키지를 선언합니다.

```lua
-- 1. 내 깃허브 저장소 주소를 원격 패키지 소스로 등록하고 불러옵니다.
add_requires("zet", {
    alias = "zet",
    url = "https://github.com/G1rmmr/zet.git",
    on_install = function (package)
        -- 사용자의 OS 환경(윈도우/맥/리눅스)에 맞춰 백그라운드에서 자동 무중단 빌드
        import("package.tools.xmake").install(package)
    end
})

-- 2. 타깃 앱에 적용하여 컴파일 없이 즉시 사용
target("my_application")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("zet")
```

---

## 3. CI/CD 파이프라인 (GitHub Actions)

본 프로젝트는 코드의 신뢰성을 증명하기 위해 모든 변경 사항에 대해 GitHub Actions에서 크로스 플랫폼 검증을 자동으로 수행합니다.

* 동작 파이프라인: 
    1. 코드 Push 또는 Pull Request 생성 시 작동
    2. `ubuntu-latest`, `macos-latest`, `windows-latest` 3대 컴파일 OS 러너 즉시 실행
    3. `actions/cache` 연동을 통해 빌드 시간 80% 단축 가속화
    4. 정적 라이브러리 빌드 및 `xmake test` 검증 일괄 완료
