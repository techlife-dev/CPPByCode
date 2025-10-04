// patterns_training.cpp
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <optional>
#include <functional>

// ---------- Domain types ----------
struct Course {
    std::string title;
    int credits = 0;
    Course() = default;
    Course(std::string t, int c) : title(std::move(t)), credits(c) {}
};
std::ostream& operator<<(std::ostream& os, Course const& c) {
    os << c.title << " (" << c.credits << "cr)";
    return os;
}

// ---------- Events for Observer ----------
enum class EventType { AttendanceMarked, FeedbackPosted, SessionStarted, SessionEnded };
struct Event {
    EventType type;
    int participantId = 0;   // optional participant target
    double value = 0.0;     // optional numeric payload (feedback etc.)
};

// ---------- Grading Strategy (Strategy) ----------
struct GradingStrategy {
    virtual ~GradingStrategy() = default;
    virtual double compute(std::vector<int> const& grades) const = 0;
};

struct AverageGrading : GradingStrategy {
    double compute(std::vector<int> const& grades) const override {
        if (grades.empty()) return 0.0;
        double s = std::accumulate(grades.begin(), grades.end(), 0.0);
        return s / grades.size();
    }
};

struct WeightedGrading : GradingStrategy {
    // For demo: apply weights 0.7 (major) + 0.3 (minor) if two grades; fallback to avg
    double compute(std::vector<int> const& grades) const override {
        if (grades.empty()) return 0.0;
        if (grades.size() == 2) return grades[0]*0.7 + grades[1]*0.3;
        return AverageGrading{}.compute(grades);
    }
};

// ---------- Participants (Abstract) ----------
class Participants {
protected:
    int id_ = 0;
    std::string name_;
    std::string email_;
public:
    Participants() = default;
    Participants(int id, std::string name, std::string email = {})
        : id_(id), name_(std::move(name)), email_(std::move(email)) {}
    virtual ~Participants() = default;

    int id() const noexcept { return id_; }
    std::string const& name() const noexcept { return name_; }

    virtual void printSummary(std::ostream& os = std::cout) const = 0;
    virtual double computeScore() const = 0;

    // polymorphic copy
    virtual std::unique_ptr<Participants> clone() const = 0;
};

// ---------- Student ----------
class Student : public Participants {
    std::vector<Course> courses_;
    std::vector<int> grades_;
    std::shared_ptr<GradingStrategy> strategy_;
public:
    Student() = default;
    Student(int id, std::string name, std::string email = {},
            std::vector<Course> courses = {}, std::vector<int> grades = {},
            std::shared_ptr<GradingStrategy> strat = std::make_shared<AverageGrading>())
        : Participants(id, std::move(name), std::move(email)),
          courses_(std::move(courses)), grades_(std::move(grades)), strategy_(std::move(strat)) {}

    void addCourse(Course c, int grade = 0) {
        courses_.push_back(std::move(c));
        grades_.push_back(grade);
    }
    void setStrategy(std::shared_ptr<GradingStrategy> strat) { strategy_ = std::move(strat); }

    double computeScore() const override {
        if (strategy_) return strategy_->compute(grades_);
        return AverageGrading{}.compute(grades_);
    }

    void printSummary(std::ostream& os = std::cout) const override {
        os << "Student{id=" << id_ << ", name=\"" << name_ << "\", courses=[";
        for (size_t i = 0; i < courses_.size(); ++i) {
            os << courses_[i] << ":" << grades_[i];
            if (i + 1 < courses_.size()) os << ", ";
        }
        os << "], score=" << computeScore() << "}\n";
    }

    std::unique_ptr<Participants> clone() const override {
        return std::make_unique<Student>(*this);
    }
};

// ---------- Instructor ----------
class Instructor : public Participants {
    std::string expertise_;
    double feedback_ = 0.0;
public:
    Instructor() = default;
    Instructor(int id, std::string name, std::string email, std::string expertise, double feedback = 0.0)
        : Participants(id, std::move(name), std::move(email)), expertise_(std::move(expertise)), feedback_(feedback) {}

    double computeScore() const override { return feedback_; }

    void printSummary(std::ostream& os = std::cout) const override {
        os << "Instructor{id=" << id_ << ", name=\"" << name_ << "\", expertise=\"" << expertise_ << "\""
           << ", feedback=" << feedback_ << "}\n";
    }

    std::unique_ptr<Participants> clone() const override {
        return std::make_unique<Instructor>(*this);
    }
};

// ---------- Observer interface ----------
struct IObserver {
    virtual ~IObserver() = default;
    virtual void onNotify(Event const& e) = 0;
};

// Make Observer participant (QA/monitor) which is also a Participant and an IObserver
class QAObserver : public Participants, public IObserver {
    std::string team_;
public:
    QAObserver() = default;
    QAObserver(int id, std::string name, std::string email, std::string team = "QA")
        : Participants(id, std::move(name), std::move(email)), team_(std::move(team)) {}

    void onNotify(Event const& e) override {
        if (e.type == EventType::AttendanceMarked) {
            std::cout << "[QAObserver:" << id_ << "] noticed attendance for id=" << e.participantId << "\n";
        } else if (e.type == EventType::FeedbackPosted) {
            std::cout << "[QAObserver:" << id_ << "] feedback=" << e.value << " from id=" << e.participantId << "\n";
        }
    }

    double computeScore() const override { return 0.0; } // not meaningful
    void printSummary(std::ostream& os = std::cout) const override {
        os << "QAObserver{id=" << id_ << ", name=\"" << name_ << "\", team=\"" << team_ << "\"}\n";
    }
    std::unique_ptr<Participants> clone() const override { return std::make_unique<QAObserver>(*this); }
};

// ---------- ParticipantFactory (Factory) ----------
struct ParticipantFactory {
    static std::unique_ptr<Participants> createStudent(int id, std::string name, std::string email,
                                                       std::vector<Course> courses = {},
                                                       std::vector<int> grades = {},
                                                       std::shared_ptr<GradingStrategy> strat = nullptr) {
        if (!strat) strat = std::make_shared<AverageGrading>();
        return std::make_unique<Student>(id, std::move(name), std::move(email),
                                         std::move(courses), std::move(grades), std::move(strat));
    }
    static std::unique_ptr<Participants> createInstructor(int id, std::string name, std::string email,
                                                          std::string expertise, double feedback = 0.0) {
        return std::make_unique<Instructor>(id, std::move(name), std::move(email), std::move(expertise), feedback);
    }
    static std::unique_ptr<Participants> createQAObserver(int id, std::string name, std::string email, std::string team = "QA") {
        return std::make_unique<QAObserver>(id, std::move(name), std::move(email), std::move(team));
    }
};

// ---------- ParticipantRepository (Repository / DAO) ----------
class ParticipantRepository {
    std::unordered_map<int, std::unique_ptr<Participants>> store_;
public:
    void save(std::unique_ptr<Participants> p) {
        if (!p) return;
        int id = p->id();
        store_[id] = std::move(p);
    }
    Participants* find(int id) const {
        auto it = store_.find(id);
        return it == store_.end() ? nullptr : it->second.get();
    }
    std::unique_ptr<Participants> extract(int id) {
        auto it = store_.find(id);
        if (it == store_.end()) return nullptr;
        auto ptr = std::move(it->second);
        store_.erase(it);
        return ptr;
    }
    std::vector<Participants*> all() const {
        std::vector<Participants*> out;
        out.reserve(store_.size());
        for (auto const& kv : store_) out.push_back(kv.second.get());
        return out;
    }
};

// ---------- Session (Subject / Mediator) ----------
class Session {
    int id_;
    std::string title_;
    ParticipantRepository* repo_; // non-owning repo reference
    std::vector<IObserver*> observers_;
    std::unordered_map<int, bool> attendance_; // studentId -> present?
public:
    Session(int id, std::string title, ParticipantRepository* repo)
        : id_(id), title_(std::move(title)), repo_(repo) {}

    void attach(IObserver* o) {
        if (!o) return;
        observers_.push_back(o);
    }
    void detach(IObserver* o) {
        observers_.erase(std::remove(observers_.begin(), observers_.end(), o), observers_.end());
    }
    void notify(Event const& e) {
        for (auto* o : observers_) if (o) o->onNotify(e);
    }

    bool markAttendance(int studentId) {
        Participants* p = repo_->find(studentId);
        if (!p) {
            std::cout << "[Session] student id " << studentId << " unknown\n";
            return false;
        }
        attendance_[studentId] = true;
        notify(Event{EventType::AttendanceMarked, studentId, 1.0});
        return true;
    }
    bool unmarkAttendance(int studentId) {
        auto it = attendance_.find(studentId);
        if (it == attendance_.end()) return false;
        attendance_.erase(it);
        notify(Event{EventType::AttendanceMarked, studentId, 0.0});
        return true;
    }

    void postFeedback(int participantId, double feedbackScore) {
        notify(Event{EventType::FeedbackPosted, participantId, feedbackScore});
    }

    void printAttendance() const {
        std::cout << "Attendance for session '" << title_ << "':\n";
        for (auto const& kv : attendance_) std::cout << "  id=" << kv.first << " present\n";
    }
};

// ---------- Command (Command pattern) ----------
struct ICommand {
    virtual ~ICommand() = default;
    virtual bool execute() = 0;
    virtual void undo() = 0;
};

class MarkAttendanceCommand : public ICommand {
    Session* session_;
    int studentId_;
    bool executed_{false};
public:
    MarkAttendanceCommand(Session* s, int id) : session_(s), studentId_(id) {}
    bool execute() override {
        if (!session_) return false;
        executed_ = session_->markAttendance(studentId_);
        return executed_;
    }
    void undo() override {
        if (executed_ && session_) {
            session_->unmarkAttendance(studentId_);
            executed_ = false;
        }
    }
};

// Simple invoker with undo stack
class CommandInvoker {
    std::vector<std::unique_ptr<ICommand>> history_;
public:
    bool run(std::unique_ptr<ICommand> cmd) {
        if (!cmd) return false;
        bool ok = cmd->execute();
        if (ok) history_.push_back(std::move(cmd));
        return ok;
    }
    void undoLast() {
        if (history_.empty()) return;
        auto cmd = std::move(history_.back());
        history_.pop_back();
        cmd->undo();
    }
};

// ---------- SessionBuilder & TrainingFacade ----------
class SessionBuilder {
    int id_ = 0;
    std::string title_ = "untitled";
    ParticipantRepository* repo_ = nullptr;
public:
    SessionBuilder& setId(int id) { id_ = id; return *this; }
    SessionBuilder& setTitle(std::string t) { title_ = std::move(t); return *this; }
    SessionBuilder& setRepo(ParticipantRepository* r) { repo_ = r; return *this; }
    std::unique_ptr<Session> build() {
        return std::make_unique<Session>(id_, std::move(title_), repo_);
    }
};

class TrainingFacade {
    ParticipantRepository repo_;
public:
    ParticipantRepository* repo() { return &repo_; }

    // Enroll convenience: wraps factory + repo
    Participants* enrollStudent(int id, std::string name, std::string email,
                                std::vector<Course> courses = {}, std::vector<int> grades = {}) {
        auto p = ParticipantFactory::createStudent(id, std::move(name), std::move(email), std::move(courses), std::move(grades));
        Participants* out = p.get();
        repo_.save(std::move(p));
        return out;
    }

    Participants* enrollInstructor(int id, std::string name, std::string email, std::string expertise, double score=0.0) {
        auto p = ParticipantFactory::createInstructor(id, std::move(name), std::move(email), std::move(expertise), score);
        Participants* out = p.get();
        repo_.save(std::move(p));
        return out;
    }

    Participants* enrollQAObserver(int id, std::string name, std::string email, std::string team="QA") {
        auto p = ParticipantFactory::createQAObserver(id, std::move(name), std::move(email), std::move(team));
        Participants* out = p.get();
        repo_.save(std::move(p));
        return out;
    }
};

// ---------- Demo main() tying everything together ----------
int main() {
    std::cout << "=== Training Patterns Demo ===\n\n";

    TrainingFacade facade;

    // Enroll participants via facade/factory
    facade.enrollStudent(101, "Alice", "alice@x.com", {Course("C++ Basics",3)}, {85});
    facade.enrollStudent(102, "Bob", "bob@x.com", {Course("C++ Basics",3)}, {72});
    facade.enrollInstructor(201, "Dr. Carol", "carol@uni.edu", "Advanced C++", 4.8);
    facade.enrollQAObserver(301, "Eve", "eve@qa.org", "QA Team");

    // Build a session (Builder) that uses repository
    SessionBuilder sb;
    auto session = sb.setId(1).setTitle("C++ 101 - Morning").setRepo(facade.repo()).build();

    // Attach QA observer(s) to session (Observer)
    Participants* q = facade.repo()->find(301);
    if (q) {
        // QAObserver also implements IObserver (downcast)
        if (auto qa = dynamic_cast<IObserver*>(q)) session->attach(qa);
    }

    // Command invoker
    CommandInvoker invoker;

    std::cout << "\n--- Mark attendance with Command and Observer notifications ---\n";
    invoker.run(std::make_unique<MarkAttendanceCommand>(session.get(), 101)); // Alice
    invoker.run(std::make_unique<MarkAttendanceCommand>(session.get(), 102)); // Bob

    session->printAttendance();

    std::cout << "\n--- Undo last attendance (Bob) ---\n";
    invoker.undoLast();
    session->printAttendance();

    // Strategy: swap grading for Alice to WeightedGrading
    if (auto sp = dynamic_cast<Student*>(facade.repo()->find(101))) {
        sp->setStrategy(std::make_shared<WeightedGrading>());
        sp->addCourse(Course("Advanced Topics", 4), 92); // add another grade to show weighted effect
        std::cout << "\nAfter changing grading strategy and adding course to Alice:\n";
        sp->printSummary();
    }

    // Clone example (Prototype) — deep copy polymorphically
    std::cout << "\n--- Cloning Alice and enrolling clone as id=501 ---\n";
    if (auto original = facade.repo()->find(101)) {
        auto clonePtr = original->clone();              // polymorphic clone
        // adjust id and name (we need a way; clone returns Participants — assume derived copy)
        // For demo we dynamically cast and mutate if Student
        if (auto sClone = dynamic_cast<Student*>(clonePtr.get())) {
            // set new id via hacky approach: create new Student from clone (safer to implement setId in real code)
            // We'll create a new Student using copy-construction and then replace id_ by re-creating
            auto newStudent = std::make_unique<Student>(*sClone);
            // replace id/name by constructing new object
            newStudent->printSummary();
            // rewrap and save under new repo id
            // For simplicity, save as-is but understand in real system you'd allow mutators or builder pattern.
            // Here just demonstrate clone() usage:
            std::cout << "[Demo] clone produced student (id preserved in clone). Not changing id in this demo.\n";
        }
        facade.repo()->save(std::move(clonePtr)); // will store under same id if not changed (overwrites); ok for demo
    }

    std::cout << "\n--- Session posting feedback ---\n";
    session->postFeedback(201, 4.2); // instructor feedback posted -> QAObserver notified

    std::cout << "\n=== End Demo ===\n";
    return 0;
}
