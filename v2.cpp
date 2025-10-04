#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>

// ---------- Course ----------
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

// ---------- Abstract base: Participants ----------
class Participants {
protected:
    int id_ = 0;
    std::string name_;
    std::string email_;

public:
    Participants() = default;
    Participants(int id, std::string name, std::string email = {})
        : id_(id), name_(std::move(name)), email_(std::move(email)) {}

    Participants(Participants const&) = default;
    Participants& operator=(Participants const&) = default;
    Participants(Participants&&) noexcept = default;
    Participants& operator=(Participants&&) noexcept = default;

    virtual ~Participants() = default;                 // virtual dtor for polymorphic cleanup

    int id() const noexcept { return id_; }
    std::string const& name() const noexcept { return name_; }
    std::string const& email() const noexcept { return email_; }

    // polymorphic interface
    virtual void printSummary(std::ostream& os = std::cout) const = 0;
    virtual double computeScore() const = 0;

    // clone to create a polymorphic copy (returns unique_ptr)
    virtual std::unique_ptr<Participants> clone() const = 0;
};

// ---------- Derived: Student ----------
class Student : public Participants {
    std::vector<Course> courses_;
    std::vector<int> grades_; // parallel to courses_: grade per course (0-100)

public:
    Student() = default;
    Student(int id, std::string name, std::string email,
            std::vector<Course> courses = {}, std::vector<int> grades = {})
        : Participants(id, std::move(name), std::move(email)),
          courses_(std::move(courses)),
          grades_(std::move(grades))
    {}

    void addCourse(Course c, int grade = 0) {
        courses_.push_back(std::move(c));
        grades_.push_back(grade);
    }

    double computeScore() const override {
        if (grades_.empty()) return 0.0;
        double sum = std::accumulate(grades_.begin(), grades_.end(), 0.0);
        return sum / static_cast<double>(grades_.size());
    }

    void printSummary(std::ostream& os = std::cout) const override {
        os << "Student{id=" << id_ << ", name=\"" << name_ << "\", email=\"" << email_
           << "\", courses=[";
        for (size_t i = 0; i < courses_.size(); ++i) {
            os << courses_[i] << ":" << grades_[i];
            if (i + 1 < courses_.size()) os << ", ";
        }
        os << "], avgGrade=" << computeScore() << "}\n";
    }

    std::unique_ptr<Participants> clone() const override {
        return std::make_unique<Student>(*this); // uses default copy (vector copies)
    }
};

// ---------- Derived: Instructor ----------
class Instructor : public Participants {
    std::string expertise_;
    double feedbackScore_ = 0.0; // 0..5
public:
    Instructor() = default;
    Instructor(int id, std::string name, std::string email,
               std::string expertise, double feedback = 0.0)
        : Participants(id, std::move(name), std::move(email)),
          expertise_(std::move(expertise)), feedbackScore_(feedback)
    {}

    double computeScore() const override {
        return feedbackScore_;
    }

    void printSummary(std::ostream& os = std::cout) const override {
        os << "Instructor{id=" << id_ << ", name=\"" << name_ << "\", email=\"" << email_
           << "\", expertise=\"" << expertise_ << "\", feedback=" << feedbackScore_ << "}\n";
    }

    std::unique_ptr<Participants> clone() const override {
        return std::make_unique<Instructor>(*this);
    }
};

// ---------- Demo ----------
int main() {
    using Ptr = std::unique_ptr<Participants>;
    std::vector<Ptr> roster;

    // create student
    auto s = std::make_unique<Student>(101, "Alice", "alice@example.com");
    s->addCourse(Course("C++ Basics", 3), 85);
    s->addCourse(Course("Data Structures", 4), 90);

    // create instructor
    auto ins = std::make_unique<Instructor>(201, "Dr. Bob", "bob@uni.edu", "Advanced C++", 4.7);

    // move into roster
    roster.push_back(std::move(s));
    roster.push_back(std::move(ins));

    std::cout << "=== Roster (polymorphic iteration) ===\n";
    for (auto const& p : roster) {
        p->printSummary();
        std::cout << "  Score: " << p->computeScore() << "\n";
    }

    // clone a participant (deep copy) and push the clone into roster
    std::cout << "\n=== Cloning first participant and adding clone to roster ===\n";
    if (!roster.empty()) {
        roster.push_back(roster[0]->clone()); // deep copy via clone()
    }

    // demonstrate searching by id
    int searchId = 101;
    auto it = std::find_if(roster.begin(), roster.end(),
        [searchId](auto const& p){ return p->id() == searchId; });

    if (it != roster.end()) {
        std::cout << "\nFound participant with id " << searchId << ":\n";
        (*it)->printSummary();
    }

    // demonstrate moving ownership out of vector (extract and operate)
    std::cout << "\n=== Extract (move) a participant out of roster ===\n";
    Ptr extracted = std::move(roster.back()); // moves unique_ptr out (roster.back() becomes null)
    roster.pop_back();                         // remove the (now-null) slot
    if (extracted) {
        std::cout << "Extracted:\n";
        extracted->printSummary();
    }

    std::cout << "\n=== End main: unique_ptr will cleanup automatically ===\n";
    return 0;
}
