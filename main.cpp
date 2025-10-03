#include <iostream>
#include <string>
#include <vector>
#include <utility>   // std::move
#include <numeric>   // accumulate
#include <initializer_list>

// Simple Course
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

// Participant using std::vector (Rule of Zero)
class Participant {
private:
    int id_ = 0;
    std::string name_;
    std::vector<Course> courses_;   // managed by vector

public:
    // defaulted special members - Rule of Zero (no manual copy/move/dtor)
    Participant() = default;

    Participant(int id, std::string name, std::initializer_list<Course> courses = {})
        : id_(id), name_(std::move(name)), courses_(courses)
    {}

    // convenience: construct from vector
    Participant(int id, std::string name, std::vector<Course> courses)
        : id_(id), name_(std::move(name)), courses_(std::move(courses))
    {}

    // behavior
    void addCourse(Course const& c) { courses_.push_back(c); }
    void addCourse(Course&& c) { courses_.push_back(std::move(c)); }

    std::size_t courseCount() const noexcept { return courses_.size(); }

    // compute total credits as example of a member operation
    int totalCredits() const noexcept {
        return std::accumulate(courses_.begin(), courses_.end(), 0,
            [](int sum, Course const& c){ return sum + c.credits; });
    }

    friend std::ostream& operator<<(std::ostream& os, Participant const& p) {
        os << "Participant{id=" << p.id_ << ", name=\"" << p.name_
           << "\", courses=[";
        for (std::size_t i = 0; i < p.courses_.size(); ++i) {
            os << p.courses_[i];
            if (i + 1 < p.courses_.size()) os << ", ";
        }
        os << "], totalCredits=" << p.totalCredits() << "}";
        return os;
    }
};

int main() {
    std::cout << "=== Create p1 with two courses ===\n";
    Participant p1(101, "Alice", { Course("C++ Basics", 3), Course("Data Structures", 4) });
    std::cout << "p1: " << p1 << "\n\n";

    std::cout << "=== Copy construct p2 from p1 (deep copy of vector elements) ===\n";
    Participant p2 = p1;                      // copy ctor (vector elements copied)
    std::cout << "p1: " << p1 << "\n";
    std::cout << "p2: " << p2 << "\n\n";

    std::cout << "=== Modify p2 by adding a course (should not affect p1) ===\n";
    p2.addCourse(Course("Algorithms", 3));
    std::cout << "p1: " << p1 << "\n";
    std::cout << "p2: " << p2 << "\n\n";

    std::cout << "=== Copy-assign p3 = p2 ===\n";
    Participant p3;
    p3 = p2;                                  // copy assignment (vector copied)
    std::cout << "p3: " << p3 << "\n\n";

    std::cout << "=== Move construct p4 from p3 ===\n";
    Participant p4 = std::move(p3);           // move ctor (vector moved, p3 left empty)
    std::cout << "p4: " << p4 << "\n";
    std::cout << "p3 (after move): " << p3 << "\n\n";

    std::cout << "=== Move assign p5 = std::move(p4) ===\n";
    Participant p5;
    p5 = std::move(p4);                       // move assignment (vector moved)
    std::cout << "p5: " << p5 << "\n";
    std::cout << "p4 (after move): " << p4 << "\n\n";

    std::cout << "=== End of main: destructors for automatic objects run (vector cleans itself) ===\n";
    return 0;
}
